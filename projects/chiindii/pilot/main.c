#include <util/delay.h>
#include "main.h"

#define HEARTBEAT_OVERFLOW	32
#define BATTERY_OVERFLOW	4

int main(){
	//********************
	// Module Init
	//********************

	status_init();
	comm_init();
	timer_init();
	gyro_init();
	esc_init();
	accel_init();  //We call accel_init after all other interrupt-driven modules, as it forces sei() (without which i2c will hang).
	pid_init();
	motor_init();
	attitude_init(gyro_get(), accel_get());
	
	if (esc_calibration_required()){
		esc_calibrate();
	}

	//Flash the startup status for 500ms (in direct status implementation, all 5 LEDs are on).
	status_error(STATUS_ERR_RESET);
	_delay_ms(500);
	//Statup status is cleared after all init is completed

	//********************
	// Variable Declarations
	//********************

	//Time variables	
	double seconds = 0;				//Last measured seconds (with close-to-microsecond resolution)
	double curr_seconds;			//Current seconds
	double dt;						//Time since last loop execution (how long the loop took; used for time sensitive calculations like Kalman)

	//State variables
	double throttle = 0.0;			//Throttle as sent from controller
	vector_t sp = { 0,0,0 };		//Attitude set point
	uint8_t cmd_type;				//Can be any of the flight command types (a superset of armed_type)
	uint8_t armed = 0;				//Boolean; 0 = not armed, !0 = armed
	double armed_timeout = 0.0;		//Delta time in seconds since last 'A' command recieved
	double flight_command_data[4];	//Data packet received with a flight command
	double motor[4];				//Motor set point; used as an output from the motor module and input to ESC module
	
	uint8_t heartbeat_overflow = 0;	//This increments on every loop; when this reaches HEARTBEAT_OVERFLOW (approx 1/5 second) we send heartbeat and telemetry
	uint8_t battery_overflow = 0;	//This is incremented every time heartbeat_overflow overflows (i.e. just under every second); when this gets to 6 (about 3 seconds) we send battery info


	//Variables used in calculations
	vector_t g;						//Gyro readings
	vector_t a;						//Accel readings
	vector_t pv;					//Process Variable
	vector_t mv;					//Manipulated Variable (output of PID)
	
	
	//Clear startup status
	status_error(0x00);	

	//Main program loop
	while (1) {
		protocol_poll();
		cmd_type = protocol_receive_flight_command(flight_command_data);
		if (cmd_type == 'A') {
			armed_timeout = 0.0;
			armed = 1;

			throttle = flight_command_data[0] * 0.8;	// scale the requested value to allow for manoeverability at 100% throttle
			sp.x = flight_command_data[1];
			sp.y = flight_command_data[2];
			sp.z = flight_command_data[3];
		}
		else if (cmd_type == 'M') {
			armed = 0;
		}

		g = gyro_get();
		a = accel_get();

		//Compute dt as close to use time as possible
		curr_seconds = timer_micros() / 1000000.0;
		dt = curr_seconds - seconds;
		seconds = curr_seconds;
		
		pv = attitude(g, a, dt);					// compute PID process variable for x and y using Kalman

		if (armed) {
			armed_timeout += dt;
			status_set(STATUS_ARMED);
			
			mv = pid_mv(sp, pv, dt);					// PID manipulated variable
			
			motor_percent(throttle, mv, motor);
			esc_set(motor);
			
			//Reset PID if throttle is 0.
			if (throttle < 0.01) {
				pid_reset();
			}
			
			if (armed_timeout > 1.0){
				// level out 
				sp.x = 0.0;
				sp.y = 0.0;
				sp.z = 0.0;
				
				//This should go from full throttle to off in about 10 seconds.
				if (throttle > 0.0) {
					throttle = throttle - 0.1;
				}
			
				//Verify that throttle is not negative
				if (throttle <= 0.0) {
					throttle = 0.0;
				}
				
				armed_timeout = 0.0;
			}
		}
		else {
			status_clear(STATUS_ARMED);
			
			//Reset PID, since it is not armed
			pid_reset();

			//Kill throttle
			throttle = 0.0;
			motor[0] = 0.0;
			motor[1] = 0.0;
			motor[2] = 0.0;
			motor[3] = 0.0;
			esc_set(motor);
		}

		//********************
		// Intermittent I/O
		//********************

		heartbeat_overflow++;
		if (heartbeat_overflow >= HEARTBEAT_OVERFLOW){
			heartbeat_overflow = 0;
			
			status_toggle(STATUS_HEARTBEAT);
			
			status_clear(STATUS_MESSAGE_RX);
			status_clear(STATUS_MESSAGE_TX);
			
			protocol_send_telemetry(pv, motor);
			protocol_send_raw(g, a);
			
			battery_overflow++;
			if (battery_overflow >= BATTERY_OVERFLOW){
				battery_overflow = 0;
				protocol_send_battery(battery_voltage());
				protocol_send_version(GIT_VERSION);
			}
		}
	}
}
