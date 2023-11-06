#include <ros.h>
#include <std_msgs/String.h>
#include <geometry_msgs/Twist.h>
#include "math.h"
#include <std_msgs/Int16.h>
#include <std_msgs/Float64.h>

#define ENC_IN_LEFT_A 2
#define ENC_IN_RIGHT_A 3
#define ENC_IN_LEFT_B 4
#define ENC_IN_RIGHT_B 11
 
boolean Direction_left = true;
boolean Direction_right = true;

const int encoder_minimum = -32768;
const int encoder_maximum = 32767;

std_msgs::Float64 linear_velocity_error_integral = 0.0;
std_msgs::Float64 angular_velocity_error_integral = 0.0;

std_msgs::Int16 right_wheel_tick_count;
ros::Publisher rightPub("right_ticks", &right_wheel_tick_count);
std_msgs::Int16 left_wheel_tick_count;
ros::Publisher leftPub("left_ticks", &left_wheel_tick_count);

geometry_msgs::Twist cmd_vel;
ros::Publisher motor_control_pub("motor_control", &cmd_vel);

const int interval = 30;
long previousMillis = 0;
long currentMillis = 0;

int pwmReq = 0;

ros::NodeHandle nh;

double Kp_linear = 1.0;
double Ki_linear = 0.1;
double Kd_linear = 0.01;

double Kp_angular = 1.0;
double Ki_angular = 0.1;
double Kd_angular = 0.01;

void right_wheel_tick() {
   
  int val = digitalRead(ENC_IN_RIGHT_B);
 
  if (val == LOW) {
    Direction_right = false; 
  }
  else {
    Direction_right = true;
  }
   
  if (Direction_right) {
     
    if (right_wheel_tick_count.data == encoder_maximum) {
      right_wheel_tick_count.data = encoder_minimum;
    }
    else {
      right_wheel_tick_count.data++;  
    }    
  }
  else {
    if (right_wheel_tick_count.data == encoder_minimum) {
      right_wheel_tick_count.data = encoder_maximum;
    }
    else {
      right_wheel_tick_count.data--;  
    }  
  }
}
 

void left_wheel_tick() {
   
  int val = digitalRead(ENC_IN_LEFT_B);
 
  if (val == LOW) {
    Direction_left = true; 
  }
  else {
    Direction_left = false; 
  }
   
  if (Direction_left) {
    if (left_wheel_tick_count.data == encoder_maximum) {
      left_wheel_tick_count.data = encoder_minimum;
    }
    else {
      left_wheel_tick_count.data++;  
    }  
  }
  else {
    if (left_wheel_tick_count.data == encoder_minimum) {
      left_wheel_tick_count.data = encoder_maximum;
    }
    else {
      left_wheel_tick_count.data--;  
    }  
  }
}


void drive(int Lpwm, int Rpwm ,int an1 ,int an2 ,int an3 ,int an4){
analogWrite(9, Lpwm);
analogWrite(10, Rpwm);
digitalWrite(5, an1);
digitalWrite(6, an2);
digitalWrite(7, an3);
digitalWrite(8, an4);}


void dc_driver(const geometry_msgs::Twist& msg){
// Calculate time difference
static ros::Time previous_time = nh.now();
ros::Time current_time = nh.now();
double delta_time = (current_time - previous_time).toSec();

  // Calculate linear and angular velocity errors
double linear_velocity_error = msg.linear.x - cmd_vel.linear.x;
double angular_velocity_error = msg.angular.z - cmd_vel.angular.z;

  // Calculate time derivative (D) terms
double linear_velocity_error_derivative = (linear_velocity_error - cmd_vel.linear.x) / delta_time;
double angular_velocity_error_derivative = (angular_velocity_error - cmd_vel.angular.z) / delta_time;
cmd_vel.linear.x = Kp_linear * linear_velocity_error + Ki_linear * linear_velocity_error_integral + Kd_linear * linear_velocity_error_derivative;
cmd_vel.angular.z = Kp_angular * angular_velocity_error + Ki_angular * angular_velocity_error_integral + Kd_angular * angular_velocity_error_derivative;
double a = cmd_vel.linear.x;
double c = cmd_vel.angular.z;

if(a>0 && c==0){ //straight
drive(80,80,1,0,0,1);
}

if(a<0 && c==0){ //backward
drive(80,80,0,1,1,0);
}

if(a==0 && c==0){ //stop
drive(0,0,0,0,0,0);
}
if(a==0 && c>0){ //right
drive(80,80,0,1,0,1);   
}
if(a==0 && c<0){ //left
drive(80,80,1,0,1,0);
}
linear_velocity_error_integral += linear_velocity_error * delta_time;
angular_velocity_error_integral += angular_velocity_error * delta_time;

previous_time = current_time;
motor_control_pub.publish(&cmd_vel);
}
ros::Subscriber<geometry_msgs::Twist> sub("cmd_vel", dc_driver); 


void setup()
{
pinMode(ENC_IN_LEFT_A, INPUT_PULLUP);
pinMode(ENC_IN_LEFT_B, INPUT);
pinMode(ENC_IN_RIGHT_A, INPUT_PULLUP);
pinMode(ENC_IN_RIGHT_B, INPUT);

pinMode(10, OUTPUT); //ENB
pinMode(8, OUTPUT); //IN 4
pinMode(7, OUTPUT); //IN 3 
pinMode(6, OUTPUT); //IN 2
pinMode(5, OUTPUT); //IN 1
pinMode(9, OUTPUT); //ENA 

attachInterrupt(digitalPinToInterrupt(ENC_IN_LEFT_A), left_wheel_tick, RISING);
attachInterrupt(digitalPinToInterrupt(ENC_IN_RIGHT_A), right_wheel_tick, RISING);
nh.getHardware()->setBaud(57600);
nh.initNode();
nh.advertise(rightPub);
nh.advertise(leftPub);
nh.advertise(motor_control_pub);
nh.subscribe(sub);
}

void loop() {
nh.spinOnce();
currentMillis = millis();
leftPub.publish( &left_wheel_tick_count );
rightPub.publish( &right_wheel_tick_count );

  if (currentMillis - previousMillis > interval) {
    previousMillis = currentMillis;
    leftPub.publish( &left_wheel_tick_count );
    rightPub.publish( &right_wheel_tick_count );
 
  }
 
}
