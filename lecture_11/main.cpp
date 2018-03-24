#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <libv4l2.h>

#define CLEAR(x) memset(&(x), 0, sizeof(x))

using namespace std;

static volatile int keepRunning = 1;

void intHandler(int signal) {
    keepRunning = 0;
}

struct buffer {
    void   *start;
    size_t length;
};

static void xioctl(int fh, long request, void *arg)
{
    int r;

    do {
        r = v4l2_ioctl(fh, request, arg);
    } while (r == -1 && ((errno == EINTR) || (errno == EAGAIN)));

    if (r == -1) {
        fprintf(stderr, "error %d, %s\n", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char **argv)
{
    int                             r_limit, g_limit, b_limit, threshold;
    struct v4l2_format              fmt;
    struct v4l2_buffer              buf;
    struct v4l2_requestbuffers      req;
    enum v4l2_buf_type              type;
    fd_set                          fds;
    struct timeval                  tv;
    int                             r;
    unsigned int                    n_buffers;
    struct buffer                   *buffers;

    signal(SIGINT, intHandler);

    if(argc != 5) {
        printf("./camera R G B threshold\n");
        printf("./camera 200 160 180 20\n");
        return -1;
    }
    sscanf(argv[1], "%d", &r_limit);
    sscanf(argv[2], "%d", &g_limit);
    sscanf(argv[3], "%d", &b_limit);
    sscanf(argv[4], "%d", &threshold);

    int fd = v4l2_open("/dev/video0", O_RDWR | O_NONBLOCK, 0);
    if (fd < 0) {
        perror("Cannot open device");
        exit(EXIT_FAILURE);
    }

    CLEAR(fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = 640;
    fmt.fmt.pix.height      = 480;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
    fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;
    xioctl(fd, VIDIOC_S_FMT, (void*)(&fmt));
    if (fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_RGB24) {
        printf("Libv4l didn't accept RGB24 format. Can't proceed.\n");
        exit(EXIT_FAILURE);
    }
    if ((fmt.fmt.pix.width != 640) || (fmt.fmt.pix.height != 480))
        printf("Warning: driver is sending image at %dx%d\n",
               fmt.fmt.pix.width, fmt.fmt.pix.height);

    printf("format: V4L2_PIX_FMT_RGB24\n");
    CLEAR(req);
    req.count = 2;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    xioctl(fd, VIDIOC_REQBUFS, (void*)(&req));

    printf("Buffers count: %d\n", req.count);

    buffers = (buffer*)calloc(req.count, sizeof(*buffers));
    for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
        CLEAR(buf);

        buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory      = V4L2_MEMORY_MMAP;
        buf.index       = n_buffers;

        xioctl(fd, VIDIOC_QUERYBUF, (void*)(&buf));
        buffers[n_buffers].length = buf.length;
        buffers[n_buffers].start = v4l2_mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);

        if (MAP_FAILED == buffers[n_buffers].start) {
            perror("mmap");
            exit(EXIT_FAILURE);
        }
    }

    for (unsigned int i = 0; i < n_buffers; ++i) {
        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        xioctl(fd, VIDIOC_QBUF, (void*)(&buf));
    }
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    xioctl(fd, VIDIOC_STREAMON, (void*)(&type));


    printf("Width: %d\theight: %d\n", fmt.fmt.pix.width, fmt.fmt.pix.height); 
    while(keepRunning) {
        do {
            FD_ZERO(&fds);
            FD_SET(fd, &fds);
            tv.tv_sec = 2;
            tv.tv_usec = 0;
            r = select(fd + 1, &fds, NULL, NULL, &tv);
        } while ((r == -1 && (errno = EINTR)));

        if (r == -1) {
            perror("select");
            return errno;
        }

        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        xioctl(fd, VIDIOC_DQBUF, (void*)(&buf));

        long total_r = 0;
        long total_g = 0;
        long total_b = 0;

        long found = 0;
        unsigned char* p = (unsigned char*)buffers[buf.index].start;
        for(unsigned int i = 0; i < fmt.fmt.pix.height; i++) { 
            for(unsigned int j = 0; j < fmt.fmt.pix.width; j++) { 
                unsigned char r = *(p + 0);
                unsigned char g = *(p + 1);
                unsigned char b = *(p + 2);

                total_r += r;
                total_g += g;
                total_b += b;

                int delta_r = r - r_limit;
                int delta_g = g - g_limit;
                int delta_b = b - b_limit;

                if(delta_r * delta_r + delta_g * delta_g + delta_b * delta_g <= threshold * threshold)
                    found++;
            } 
        }

        if(fmt.fmt.pix.height * fmt.fmt.pix.width / 2 < found) 
            printf("PREDMET DETEKOVAN\n");
        printf("found: %ld\n", found);
        printf("red: %f\n", (double)(255 * total_r) / ((long)fmt.fmt.pix.height * fmt.fmt.pix.width * 255));
        printf("blue: %f\n", (double)(255 * total_g) / ((long)fmt.fmt.pix.height * fmt.fmt.pix.width * 255));
        printf("green: %f\n\n", (double)(255 * total_b) / ((long)fmt.fmt.pix.height * fmt.fmt.pix.width * 255));

        xioctl(fd, VIDIOC_QBUF, (void*)(&buf));
    }



    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    xioctl(fd, VIDIOC_STREAMOFF, (void*)(&type));

    for (unsigned int i = 0; i < n_buffers; ++i)
        v4l2_munmap(buffers[i].start, buffers[i].length);
    v4l2_close(fd);

    return 0;
}
