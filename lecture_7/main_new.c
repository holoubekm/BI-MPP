#include "cpu.h"
#include "log.h"
#include "display.h"
#include "touchpad.h"
#include "usb_lib.h"

#define EP0_OUT_BUF_SIZE 64
#define EP0_IN_BUF_SIZE 64

DECLARE_EP_BEGIN
    DECLARE_EP(ep0); 
DECLARE_EP_END

DECLARE_BUFFER_BEGIN
    DECLARE_BUFFER(ep0_buf_out, EP0_OUT_BUF_SIZE);
    DECLARE_BUFFER(ep0_buf_in, EP0_IN_BUF_SIZE);
DECLARE_BUFFER_END

enum DEV_STATE {
	DEV_DETACHED,
	DEV_ATTACHED,
	DEV_POWERED,
	DEV_DEFAULT,
	DEV_ADDRESSED,
	DEV_CONFIGURED
};

enum CUR_STATE {
	CUR_HEADER,
	CUR_SEND,
	CUR_RECV,
	CUR_ACK_SEND,
	CUR_ACK_RECV,
};


const device_descr_t devdsc __attribute__((space(auto_psv))) = {
    sizeof(device_descr_t), 1, 0x0200, 0, 0, 0, 64, 0x1111, 0x2222, 0x0100, 0, 0, 0, 1
};

enum CUR_STATE state = CUR_WAIT;
enum DEV_STATE dev_state = DEV_DEFAULT;

byte buf[512];
int total_io = 0;
int total_length = 0;
int cur_data_type = USB_TRN_DATA1_IN;

void process_control_transfer(int ep) {
	//IN
	
	if(state == CUR_HEADER) {
		if(is_setup((ep & 0x80), EP(ep0))) {
			log_str("REQUEST IN\n");

			//Strana 250
			usb_device_req_t req;
			copy_from_buffer(ep0_buf_out, &req, sizeof(usb_device_req_t));
			// log_int(" 0x%02lx\n", req.bmRequestType);
			// log_int(" 0x%02lx\n", req.bRequest);
			// log_int(" 0x%04lx\n", req.wValue);
			// log_int(" 0x%04lx\n", req.wIndex);
			// log_int(" 0x%04lx\n", req.wLength);
			switch(req.bRequest) {
				case GET_DESCRIPTOR:
					log_str(" GET_DESCRIPTOR\n");
					switch(req.wValue >> 8) {
						case DSC_DEVICE:
							log_str("  DSC_DEVICE\n");
							total_length = sizeof(device_descr_t);
							total_io = 0; cur_data_type = USB_TRN_DATA1_IN;
							memcpy(buf, &devdsc, total_length);
							state = CUR_SEND;
						break;
					}
				break;
			}
		}
	}

	if(state == CUR_RECV) {
		int count = EP(ep0)->out.cnt;
		copy_from_buffer(ep0_buf_out, buf + total_io, count);
		total_io += count;

		if(count < EP0_OUT_BUF_SIZE) {
			log_str("  RECEIVED ALL\n");
			state = CUR_ACK_SEND;
		}
	}
	else if(state == CUR_SEND) {
		int count = (total_length - total_io) % EP0_IN_BUF_SIZE;
		copy_to_buffer(ep0_buf_in, buf + total_io, count);
		usb_ep_transf_start(EP(ep0), cur_data_type, ep0_buf_in, count);
		cur_data_type = cur_data_type == USB_TRN_DATA1_IN ? USB_TRN_DATA0_IN : USB_TRN_DATA1_IN;
		if(count < EP0_IN_BUF_SIZE) {
			state = CUR_ACK_RECV;
		}
	}
	else if(state == CUR_ACK_SEND) {
		usb_ep_transf_start(EP(ep0), USB_TRN_DATA1_IN, ep0_buf_in, EP0_IN_BUF_SIZE);
		state = CUR_HEADER;
	} 
	else if(state == CUR_ACK_RECV) {
		usb_ep_transf_start(EP(ep0), USB_TRN_DATA1_OUT, ep0_buf_out, EP0_OUT_BUF_SIZE);
		state = CUR_HEADER;
	} 
	else if(state == CUR_HEADER) {
		usb_ep_transf_start(EP(ep0), USB_TRN_SETUP, ep0_buf_out, EP0_OUT_BUF_SIZE);
		state = CUR_HEADER;
	}

	// } else if(ep == 0x80) {
	// 	log_str("EP 0x80");

	// 	//IN -> to PC
	// 	//Accept ACK
	// 	// usb_ep_transf_start(EP(ep0), byte USB_TRN_DATA1_OUT, buf_ptr_t buffer, int size);
	// }
}

void process_ep_transfer(int ep) {

}

int main(int argc, char* argv[]) {
	cpu_init();
	disp_init();
	touchpad_init();
	log_init();CUR_WAIT
CUR_WAIT
	while(1) {
		switch(dev_state) {
			case DEV_DETACHED:
				log_str("DEV_DETACHED\n");
				usb_init();
				while(!is_attached());
				usb_enable();
				dev_state = DEV_ATTACHED;
			break;
			case DEV_ATTACHED:
				log_str("DEV_ATTACHED\n");
				while(!is_powered());
				dev_state = DEV_POWERED;
			break;
			case DEV_POWERED:
			case DEV_DEFAULT:
			case DEV_ADDRESSED:
			case DEV_CONFIGURED:
				if(is_reset()) {
					log_str("DEV_RESET\n");
					usb_reset();
					usb_init_ep(0, EP_SETUP_INOUT, EP(ep0));

					usb_ep_transf_start(EP(ep0), USB_TRN_SETUP, ep0_buf_out, EP0_OUT_BUF_SIZE);
					state = CUR_WAIT;
					dev_state = DEV_DEFAULT;
				}

				if (is_idle()) {
					log_str("IDLE\n");
					continue;
				}

				if(is_sof()) {
					// log_str("SOF\n");
					continue;
				}
			break;
		}

		if(is_transfer_done()) {
			log_str("TRANSFER DONE\n");
			int ep_num = get_trn_status();
			if (ep_num == 0 || ep_num == 0x80) {
				process_control_transfer(ep_num);
				continue;
			}
			process_ep_transfer(ep_num);
			continue;
		}

		if(is_attached() == FALSE) {
			dev_state = DEV_DETACHED;
		}

		log_main_loop();
	}
	return 0;
}
