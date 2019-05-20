/* blinkenlight_api_server_procs.c: Implementation of Blinkenlight API on the server

 Copyright (c) 2015-2016, Joerg Hoppe
 j_hoppe@t-online.de, www.retrocmp.com

 Permission is hereby granted, free of charge, to any person obtaining a
 copy of this software and associated documentation files (the "Software"),
 to deal in the Software without restriction, including without limitation
 the rights to use, copy, modify, merge, publish, distribute, sublicense,
 and/or sell copies of the Software, and to permit persons to whom the
 Software is furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 JOERG HOPPE BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 04-Aug-2016  JH      activated bitwise input lowpass for pin debouncing (c->fmax)
 08-May-2016  JH      new event "set_controlvalue"
 22-Feb-2016  JH	  added panel mode set/get callbacks
 13-Nov-2015  JH      created


 To get a template file generated by rpcgen,
 do "make demo" in the blinkenlight_api directory

 Calls of RPC procedures are implemented with blinkenbus_* functions
 (if interfacing to physical panel)
 OR
 by calls to panelsim_* procedures
 (if interfacing a simulated panel)
 */

#define BLINKENLIGHT_API_SERVER_PROCS_C_
#include <stdlib.h>
#include <assert.h>

#include "print.h"

#include "rpc_blinkenlight_api.h"

// intern panels & controls structs
#include "blinkenlight_panels.h"

// callbacks
#include "blinkenlight_api_server_procs.h"

#include "bitcalc.h"

// global list of Blinkenlight API panels
blinkenlight_panel_list_t *blinkenlight_panel_list;

/*** call backs triggered by RPC server ***/
// called before new values for panel are sent to client
blinkenlight_api_panel_get_controlvalues_evt_t blinkenlight_api_panel_get_controlvalues_evt = NULL;
blinkenlight_api_panel_set_controlvalue_evt_t blinkenlight_api_panel_set_controlvalue_evt = NULL;
// called  after new values for panel where transmitted by client
blinkenlight_api_panel_set_controlvalues_evt_t blinkenlight_api_panel_set_controlvalues_evt = NULL;
// set/get panel state
blinkenlight_api_panel_get_state_evt_t blinkenlight_api_panel_get_state_evt = NULL;
blinkenlight_api_panel_set_state_evt_t blinkenlight_api_panel_set_state_evt = NULL;
// set get/panel mode
blinkenlight_api_panel_get_mode_evt_t blinkenlight_api_panel_get_mode_evt = NULL;
blinkenlight_api_panel_set_mode_evt_t blinkenlight_api_panel_set_mode_evt = NULL;
// get api info
blinkenlight_api_get_info_evt_t blinkenlight_api_get_info_evt;

// global flags
char blinkenlight_api_program_info[1024]; // argv[0]
char blinkenlight_api_program_name[1024]; // argv[0]
char blinkenlight_api_program_options[1024]; // argv[1..arc-1]

/*
 * 	info()
 * 	return a multi line string with info about
 * 	- server commandline
 * 	- compile date
 */

rpc_blinkenlight_api_getinfo_res*
rpc_blinkenlight_api_getinfo_1_svc(struct svc_req *rqstp)
{
	static rpc_blinkenlight_api_getinfo_res result;
	char buffer[1024];

	/*
	 * insert server code here
	 */

	// free previous result
	xdr_free((xdrproc_t) xdr_rpc_blinkenlight_api_getinfo_res, (char *) &result);
	buffer[0] = 0;

	if (blinkenlight_api_get_info_evt)
		strcat(buffer, blinkenlight_api_get_info_evt());

	// allocate new result
	result.error_code = 0;
	result.info = strdup(buffer);
	return &result;
}

/*
 * getpanelinfo()
 * return info over panel with handle "i_panel"
 * i_panel is just the index in blinkenlight_panel_list.panels[]
 * if invalid index: result.errno=1, else errno=0
 */
rpc_blinkenlight_api_getpanelinfo_res *
rpc_blinkenlight_api_getpanelinfo_1_svc(u_int i_panel, struct svc_req *rqstp)
{
	static rpc_blinkenlight_api_getpanelinfo_res result;
	blinkenlight_panel_t *p;

	print(LOG_DEBUG, "blinkenlight_api_getpanelinfo(i_panel=%d)\n", i_panel);
	if (i_panel >= blinkenlight_panel_list->panels_count) {
		print(LOG_DEBUG, "  i_panel > panels_count\n");
		result.error_code = 1;
	} else {
		p = &(blinkenlight_panel_list->panels[i_panel]);

		// free previous result
		xdr_free((xdrproc_t) xdr_rpc_blinkenlight_api_getpanelinfo_res, (char *) &result);

		// allocate new result
		result.panel.name = strdup(p->name);
		//result.panel.info = strdup(p->info);
		result.panel.controls_outputs_count = p->controls_outputs_count;
		result.panel.controls_inputs_count = p->controls_inputs_count;
		result.panel.controls_inputs_values_bytecount = p->controls_inputs_values_bytecount;
		result.panel.controls_outputs_values_bytecount = p->controls_outputs_values_bytecount;

		// return panel from definition to result. Its ust the name for the moment
		//strncpy(result.blinkenlight_api_getpanelinfo_res_u.panel.name, p->name,
		//		sizeof(result.blinkenlight_api_getpanelinfo_res_u.panel.name));
		result.error_code = 0;
		print(LOG_DEBUG, "  result.name=%s, ...\n", result.panel.name);
	}
	return &result;
}

/*
 * getcontrolinfo()
 * return info over control handle "i_c in panel "i_panel"
 * i_control is the index in blinkenlight_panel_list.panels[i_panel].controls[]
 * if invalid index: result.errno=1, else errno=0
 */
rpc_blinkenlight_api_getcontrolinfo_res *
rpc_blinkenlight_api_getcontrolinfo_1_svc(u_int i_panel, u_int i_control, struct svc_req *rqstp)
{
	static rpc_blinkenlight_api_getcontrolinfo_res result;
	blinkenlight_panel_t *p;
	blinkenlight_control_t *c;

	print(LOG_DEBUG, "blinkenlight_api_getpanelinfo(i_panel=%d, i_control=%d)\n", i_panel,
			i_control);

	if (i_panel >= blinkenlight_panel_list->panels_count) {
		print(LOG_DEBUG, "  i_panel > panels_count\n");
		result.error_code = 1; // invalid panel
	} else {
		p = &(blinkenlight_panel_list->panels[i_panel]);
		if (i_control >= p->controls_count) {
			print(LOG_DEBUG, "  i_control > controls_count\n");
			result.error_code = 1; // invalid control
		} else {
			c = &(p->controls[i_control]);
			// free previous result
			xdr_free((xdrproc_t) xdr_rpc_blinkenlight_api_getcontrolinfo_res, (char *) &result);

			// allocate new result
			result.control.name = strdup(c->name);
			result.control.is_input = c->is_input;
			result.control.type = c->type;
			result.control.radix = c->radix;
			result.control.value_bitlen = c->value_bitlen;
			result.control.value_bytelen = c->value_bytelen;

			//strncpy(result.blinkenlight_api_getcontrolinfo_res_u.control.name, c->name,
			//		sizeof(result.blinkenlight_api_getcontrolinfo_res_u.control.name));
			result.error_code = 0;
			print(LOG_DEBUG, "  result.name=%s, ...\n", result.control.name);
		}
	}

	return &result;
}

/*
 * setpanel_controlvalues()
 * Set all output controls of a panel
 * argument is a list of bytes encoding the list of values for all output controls
 * the value for each output control is build by combining the next "bytelen" bytes
 * The order in the value list is the order of controls in the panels.
 * (but indexes are not the same, output controls are mixed with input controls!)
 */
rpc_blinkenlight_api_setpanel_controlvalues_res *
rpc_blinkenlight_api_setpanel_controlvalues_1_svc(u_int i_panel,
		rpc_blinkenlight_api_controlvalues_struct valuelist, struct svc_req *rqstp)
{
	uint64_t	now_us ; // system ticks in microseconds
	static rpc_blinkenlight_api_setpanel_controlvalues_res result;
	blinkenlight_panel_t *p;
	unsigned i_control;
	unsigned char *value_byte_ptr; // index in received value byte stream
	blinkenlight_control_t *c;

	print(LOG_DEBUG, "blinkenlight_api_setpanel_controlvalues(i_panel=%d)\n", i_panel);

	now_us = historybuffer_now_us() ;

	if (i_panel >= blinkenlight_panel_list->panels_count) {
		print(LOG_ERR, "i_panel > panels_count\n");
		result.error_code = 1; // invalid panel
	} else {
		p = &(blinkenlight_panel_list->panels[i_panel]);
		// check: exakt amount of values provided?
		// "Sum of bytes" must be "sum(all controls) of value_bytelen
		if (p->controls_outputs_values_bytecount != valuelist.value_bytes.value_bytes_len) {
			print(LOG_ERR, "Error in blinkenlight_api_setpanel_controlvalues():\n");
			print(LOG_ERR,
					"Sum (Panel[%s].outputcontrols.value_bytelen) is %d, but %d values were transmitted.\n",
					p->name, p->controls_outputs_values_bytecount,
					valuelist.value_bytes.value_bytes_len);
			exit(1);
		}

		/* go through all output controls, assign value to each
		 * decode control value from the right amount of bytes
		 * */
		value_byte_ptr = valuelist.value_bytes.value_bytes_val;
		for (i_control = 0; i_control < p->controls_count; i_control++) {
			c = &(p->controls[i_control]);
			if (!c->is_input) {
				assert(
						(value_byte_ptr - valuelist.value_bytes.value_bytes_val)
								< valuelist.value_bytes.value_bytes_len);
				c->value = decode_uint64_from_bytes(value_byte_ptr, c->value_bytelen);
                // trunc to valid bits
                c->value &= BitmaskFromLen64[c->value_bitlen];

                // callback before lowpass
                if (blinkenlight_api_panel_set_controlvalue_evt)
                    blinkenlight_api_panel_set_controlvalue_evt(p, c);

                if (c->fmax > 0) {
                    // low pass requested.
                    // If fmax == 0: fast processing without ring buffer and averaging logic
                    // TODO: local lamp test should be feed changed values here, so
                    //          lamptest appearance is low passed
                    historybuffer_set_val(c->history, now_us, c->value) ;
                }
				print(LOG_DEBUG, "   control[%d].value = 0x%llx (%d bytes)\n", i_control, c->value,
						c->value_bytelen);
				value_byte_ptr += c->value_bytelen;
			}
		}
		// signal to app: "value of output control updated"
		if (blinkenlight_api_panel_set_controlvalues_evt)
			blinkenlight_api_panel_set_controlvalues_evt(p, /*force_all*/0);

		result.error_code = 0;
	}
	return &result;
}

/*
 * getpanel_controlvalues()
 * Query all input controls of a panel.
 * Result is a list of bytes encoding the list of values for all input controls.
 * The bytes for each input control is are the "bytelen" lsb for each value
 * The order in the value list is the order of controls in the panels.
 * (but indexes are not the same, output controls are mixed with input controls!)
 */

rpc_blinkenlight_api_controlvalues_struct *
rpc_blinkenlight_api_getpanel_controlvalues_1_svc(u_int i_panel, struct svc_req *rqstp)
{
    uint64_t    now_us ; // system ticks in microseconds
	static rpc_blinkenlight_api_controlvalues_struct result;
	blinkenlight_panel_t *p;
	unsigned i_control;
	unsigned char *value_byte_ptr; // index in result value byte stream
	blinkenlight_control_t *c;

	print(LOG_DEBUG, "blinkenlight_api_getpanel_controlvalues(i_panel=%d)\n", i_panel);

	now_us = historybuffer_now_us();

	if (i_panel >= blinkenlight_panel_list->panels_count)
		result.error_code = 1; // invalid panel
	else {
		p = &(blinkenlight_panel_list->panels[i_panel]);

		// free previous result
		xdr_free((xdrproc_t) xdr_rpc_blinkenlight_api_controlvalues_struct, (char *) &result);

		// signal to app: "value of input control requested"
		if (blinkenlight_api_panel_get_controlvalues_evt)
			blinkenlight_api_panel_get_controlvalues_evt(p);

		result.value_bytes.value_bytes_len = p->controls_inputs_values_bytecount;
		result.value_bytes.value_bytes_val = (u_char *) calloc(p->controls_inputs_values_bytecount,
				sizeof(u_char));
		assert(result.value_bytes.value_bytes_val);


		/* go through all input controls, assign value from each into result stream
		 * each input control puts "value_bytelen" bytes into char stream, lsb first */
		value_byte_ptr = result.value_bytes.value_bytes_val;
		for (i_control = 0; i_control < p->controls_count; i_control++) {
			c = &(p->controls[i_control]);
			if (c->is_input) {
			    uint64_t    val = 0 ;
				assert(
						(value_byte_ptr - result.value_bytes.value_bytes_val)
								< result.value_bytes.value_bytes_len);
                if (c->fmax) {
                     int i_bit ;
                    // low pass each single bit of control individually!
                    // use fmax only for pin debouncing!
                     historybuffer_get_average_vals(c->history, 1000000 / c->fmax, now_us, /*bitmode*/1);
                     // re-assemble value from low-passed bits
                     for (i_bit = 0 ; i_bit < c->value_bitlen ; i_bit++)
                         if (c->averaged_value_bits[i_bit] > 128) // > 50% ?
                             val |= (1 << i_bit) ;
                }
                else val = c->value ;

				encode_uint64_to_bytes(value_byte_ptr, val, c->value_bytelen);
				print(LOG_DEBUG, "  result.values[] += control[%d].value = 0x%llx (%d bytes)\n",
						i_control, val, c->value_bytelen);
				value_byte_ptr += c->value_bytelen; // next pos in buffer
			}
		}
		result.error_code = 0;
	}

	return &result;
}

/*
 * rpc_param_get()
 * get a parameter value of an object (bus, panel, control)
 * object handle: for "bus", always 0
 *  for "panel: no of panel in global list
 *  for control??? there is no global control list -> panel_handle * 0x1000 + control index?
 */

static void rpc_param_get_intern(rpc_param_result_struct * result,
		rpc_param_cmd_get_struct *cmd_get)
{
	blinkenlight_panel_t *p;

	result->error_code = RPC_ERR_PARAM_ILL_CLASS; // default: error
	result->object_class = cmd_get->object_class;
	result->object_handle = cmd_get->object_handle;
	result->param_handle = cmd_get->param_handle;
	result->param_value = 0;
	switch (cmd_get->object_class) {
	//case RPC_PARAM_CLASS_BUS: 	break;
	case RPC_PARAM_CLASS_PANEL:
		result->error_code = RPC_ERR_PARAM_ILL_OBJECT;
		if (cmd_get->object_handle < blinkenlight_panel_list->panels_count) {
			p = &(blinkenlight_panel_list->panels[cmd_get->object_handle]); // valid panel
			result->error_code = RPC_ERR_PARAM_ILL_PARAM;
			switch (cmd_get->param_handle) {
			case RPC_PARAM_HANDLE_PANEL_BLINKENBOARDS_STATE:
				// set tristate of all BlinkenBoards of this panel.
				// side effect to other panels on the same boards!
				if (blinkenlight_api_panel_get_state_evt)
					result->param_value = blinkenlight_api_panel_get_state_evt(p);

				result->error_code = RPC_ERR_OK;

				break;
			case RPC_PARAM_HANDLE_PANEL_MODE:
				if (blinkenlight_api_panel_get_mode_evt)
					result->param_value = blinkenlight_api_panel_get_mode_evt(p);
				else
					result->param_value = p->mode;
				result->error_code = RPC_ERR_OK;
			}

		}
		break;
//	case RPC_PARAM_CLASS_CONTROL: break;
	}
	// error code only 0 if class, object and param ok

}

rpc_param_result_struct *
rpc_param_get_1_svc(rpc_param_cmd_get_struct cmd_get, struct svc_req *rqstp)
{
	static rpc_param_result_struct result;
	rpc_param_get_intern(&result, &cmd_get); // just do it by "intern" funktion
	return &result;
}

/*
 * rpc_param_set()
 * Set a parameter for an object
 * result: param value
 */
rpc_param_result_struct *
rpc_param_set_1_svc(rpc_param_cmd_set_struct cmd_set, struct svc_req *rqstp)
{
	static rpc_param_result_struct result;
	rpc_param_cmd_get_struct cmd_get;

	blinkenlight_panel_t *p;
	//blinkenlight_control_t *c;
	//unsigned i_control;

	// only 1 thing implemented
	result.error_code = RPC_ERR_PARAM_ILL_CLASS; // default: error
	switch (cmd_set.object_class) {
	//case RPC_PARAM_CLASS_BUS: 	break;
	case RPC_PARAM_CLASS_PANEL:
		result.error_code = RPC_ERR_PARAM_ILL_OBJECT;
		if (cmd_set.object_handle < blinkenlight_panel_list->panels_count) {
			p = &(blinkenlight_panel_list->panels[cmd_set.object_handle]); // valid panel
			result.error_code = RPC_ERR_PARAM_ILL_PARAM;
			switch (cmd_set.param_handle) {
			case RPC_PARAM_HANDLE_PANEL_BLINKENBOARDS_STATE:

				// set tristate of all BlinkenBoards of this panel.
				// side effect to other panels on the same boards!
				if (blinkenlight_api_panel_set_state_evt)
					blinkenlight_api_panel_set_state_evt(p, cmd_set.param_value);

				result.error_code = RPC_ERR_OK;
				break;
			case RPC_PARAM_HANDLE_PANEL_MODE:
				// set self test/power mode for every controls

				p->mode = cmd_set.param_value;
				if (blinkenlight_api_panel_set_mode_evt)
					blinkenlight_api_panel_set_mode_evt(p, cmd_set.param_value);

				/// this server can not selftest the input controls,
				// it is controlling REAL hardware
				// Should there be a robot arm to operate the panel on selftest???
				/// write values of panel controls to BLINKENBUS.
				if (blinkenlight_api_panel_set_controlvalues_evt)
					blinkenlight_api_panel_set_controlvalues_evt(p, /*force_all*/1);
				break;
			}

		}
		break;
//	case RPC_PARAM_CLASS_CONTROL: break;
	}
	// error code only 0 if class, object and param ok

	if (result.error_code)
		return &result;
	else {
		// result: query the same parameter
		cmd_get.object_class = cmd_set.object_class;
		cmd_get.object_handle = cmd_set.object_handle;
		cmd_get.param_handle = cmd_set.param_handle;
		rpc_param_get_intern(&result, &cmd_get);

		return &result;
	}
}

rpc_test_cmdstatus_struct *
rpc_test_data_to_server_1_svc(rpc_test_data_struct data, struct svc_req *rqstp)
{
	static rpc_test_cmdstatus_struct result;
	// query all BlinkenBoards. not enabled, if one of these are is not enabled

	return &result;
}

rpc_test_data_struct *
rpc_test_data_from_server_1_svc(rpc_test_cmdstatus_struct data, struct svc_req *rqstp)
{
	static rpc_test_data_struct result;

	/*
	 * insert server code here
	 */

	return &result;
}
