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
	CUR_SETUP
};


const device_descr_t devdsc __attribute__((space(auto_psv))) = {
    sizeof(device_descr_t),
    1,
    0x0200,
    0,
    0,
    0,
    64,
    0x1111,
    0x2222,
    0x0100,
    0,
    0,
    0,
    1
};

enum CUR_STATE state = CUR_SETUP;
enum DEV_STATE dev_state = DEV_DEFAULT;

void process_control_transfer(int ep) {
	//IN
	if(is_setup((ep & 0x80), EP(ep0)) != 0) {
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
					break;
				} 
			break;
		}
	} 

	// if(ep == 0x00) {
	// 	//OUT -> from PC	
	// 	usb_device_req_t req;
	// 	// log_str("EP 0x00");
	// 	// log_str("SIZE: ");
	// 	// log_int("%ld", sizeof(usb_device_req_t));
	// 	// log_str("\n");
	// 	// copy_from_buffer(ep0_buf_out, (byte*)(&req), EP(ep0)->out.cnt);
	// 	copy_from_buffer(ep0_buf_out, (byte*)&req, sizeof(usb_device_req_t));
	// 	switch(req.bmRequestType) {
	// 		case GET_STATUS:
	// 			log_str("GET_STATUS\n");
	// 		case CLEAR_FEATURE:
	// 		case SET_FEATURE:
	// 		case SET_ADDRESS:
	// 		case GET_DESCRIPTOR:
	// 		case SET_DESCRIPTOR:
	// 		case GET_CONFIGURATION:
	// 		case SET_CONFIGURATION:
	// 		case GET_INTERFACE:
	// 		case SET_INTERFACE:
	// 		case SYNCH_FRAME:
	// 		default:
	// 			log_int("REQUEST %x\n", GET_STATUS);
	// 			log_int("REQUEST %x\n", req.bmRequestType);
	// 		// usb_ep_transf_start(EP(ep0), byte USB_TRN_DATA1_IN, buf_ptr_t buffer, int size);
	// 		break;
	// 	}

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
	log_init();
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
					state = CUR_SETUP;
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
