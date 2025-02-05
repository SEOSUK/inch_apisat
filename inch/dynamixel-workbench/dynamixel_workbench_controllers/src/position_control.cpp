/*******************************************************************************
* Copyright 2016 ROBOTIS CO., LTD.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

/* Authors: Taehun Lim (Darby) */

#include "dynamixel_workbench_controllers/position_control.h"

PositionControl::PositionControl()
    :node_handle_("")
{
  std::string device_name   = node_handle_.param<std::string>("device_name", "/dev/ttyUSB0");
  uint32_t dxl_baud_rate    = node_handle_.param<int>("baud_rate", 1000000);

  uint8_t scan_range        = node_handle_.param<int>("scan_range", 200);

  double position_p_gain	= node_handle_.param<double>("position_p_gain", 800);
  double position_i_gain	= node_handle_.param<double>("position_i_gain", 0);
  double position_d_gain	= node_handle_.param<double>("position_d_gain", 0);

  uint32_t profile_velocity     = node_handle_.param<int>("profile_velocity", 200);
  uint32_t profile_acceleration = node_handle_.param<int>("profile_acceleration", 50);

  dxl_wb_ = new DynamixelWorkbench;

  dxl_wb_->begin(device_name.c_str(), dxl_baud_rate);

  if (dxl_wb_->scan(dxl_id_, &dxl_cnt_, scan_range) != true)
  {
    ROS_ERROR("Not found Motors, Please check scan range or baud rate");
    ros::shutdown();
    return;
  }


  initMsg();

  for (int index = 0; index < dxl_cnt_; index++)
  {
    dxl_wb_->itemWrite(dxl_id_[index], "Torque_Enable", 0);
     dxl_wb_->itemWrite(dxl_id_[index], "Operating_Mode", X_SERIES_CURRENT_BASED_POSITION_CONTROL_MODE);
     dxl_wb_->itemWrite(dxl_id_[index], "Torque_Enable", 1);
  }

  //  for (int index = 0; index < dxl_cnt_; index++)
  //  dxl_wb_->jointMode(dxl_id_[index], profile_velocity, profile_acceleration);
  // 이거키면 포지션컨트롤모드임

  dxl_wb_->addSyncWrite("Goal_Position");
  dxl_wb_->addSyncRead("Present_Position");
  //dxl_wb_->addSyncRead("Present_Current");

  for (int index = 0; index < dxl_cnt_; index++)
  {
    dxl_wb_->itemWrite(dxl_id_[index], "Position_P_Gain", position_p_gain);
    dxl_wb_->itemWrite(dxl_id_[index], "Position_I_Gain", position_i_gain);
    dxl_wb_->itemWrite(dxl_id_[index], "Position_D_Gain", position_d_gain);
  //   dxl_wb_->itemWrite(dxl_id_[index], "Feedforward_1st_Gain", feedforward_1st_gain);
  //   dxl_wb_->itemWrite(dxl_id_[index], "Feedforward_2nd_Gain", feedforward_2nd_gain);
  }
  
    // dxl_wb_->itemWrite(dxl_id_[1], "Position_P_Gain", 800);
    // dxl_wb_->itemWrite(dxl_id_[1], "Position_I_Gain", 0);
    // dxl_wb_->itemWrite(dxl_id_[1], "Position_D_Gain", 1500);


  initPublisher();
  initSubscriber();
}

PositionControl::~PositionControl()
{
  for (int index = 0; index < dxl_cnt_; index++)
    dxl_wb_->itemWrite(dxl_id_[index], "Torque_Enable", 0);

  ros::shutdown();
}

void PositionControl::initMsg()
{
  printf("-----------------------------------------------------------------------\n");
  printf("  dynamixel_workbench controller; position control example             \n");
  printf("-----------------------------------------------------------------------\n");
  printf("\n");

  for (int index = 0; index < dxl_cnt_; index++)
  {
    printf("MODEL   : %s\n", dxl_wb_->getModelName(dxl_id_[index]));
    printf("ID      : %d\n", dxl_id_[index]);
    printf("\n");
  }
  printf("-----------------------------------------------------------------------\n");
}

void PositionControl::initPublisher()
{
  dynamixel_state_list_pub_ = node_handle_.advertise<dynamixel_workbench_msgs::DynamixelStateList>("dynamixel_state", 10);
  joint_states_pub_ = node_handle_.advertise<sensor_msgs::JointState>("/inch/joint_states", 10);
}

void PositionControl::initSubscriber()
{
  joint_command_sub_ = node_handle_.subscribe("/goal_dynamixel_position", 10, &PositionControl::goalJointPositionCallback, this, ros::TransportHints().tcpNoDelay());
}
double present_current = 0;
void PositionControl::dynamixelStatePublish()
{
  dynamixel_workbench_msgs::DynamixelState     dynamixel_state[dxl_cnt_];
  dynamixel_workbench_msgs::DynamixelStateList dynamixel_state_list;

  for (int index = 0; index < dxl_cnt_; index++)
  {
    dynamixel_state[index].model_name          = std::string(dxl_wb_->getModelName(dxl_id_[index]));
    dynamixel_state[index].id                  = dxl_id_[index];
    dynamixel_state[index].torque_enable       = dxl_wb_->itemRead(dxl_id_[index], "Torque_Enable");
    dynamixel_state[index].present_position    = dxl_wb_->itemRead(dxl_id_[index], "Present_Position");
    dynamixel_state[index].present_velocity    = dxl_wb_->itemRead(dxl_id_[index], "Present_Velocity");
    dynamixel_state[index].present_current     = dxl_wb_->itemRead(dxl_id_[index], "Present_Current");
    present_current = dynamixel_state[index].present_current;

    dynamixel_state[index].goal_position       = dxl_wb_->itemRead(dxl_id_[index], "Goal_Position");
    dynamixel_state[index].goal_velocity       = dxl_wb_->itemRead(dxl_id_[index], "Goal_Velocity");
    dynamixel_state[index].goal_current        = dxl_wb_->itemRead(dxl_id_[index], "Goal_Current");
    dynamixel_state[index].moving              = dxl_wb_->itemRead(dxl_id_[index], "Moving");

    dynamixel_state_list.dynamixel_state.push_back(dynamixel_state[index]);
  }
  dynamixel_state_list_pub_.publish(dynamixel_state_list);
}

void PositionControl::jointStatePublish()
{
  int32_t present_position[dxl_cnt_] = {0, };

  for (int index = 0; index < dxl_cnt_; index++)
    present_position[index] = dxl_wb_->itemRead(dxl_id_[index], "Present_Position");

  int32_t present_velocity[dxl_cnt_] = {0, };

  //for (int index = 0; index < dxl_cnt_; index++)
  //  present_velocity[index] = dxl_wb_->itemRead(dxl_id_[index], "Present_Velocity");

  int16_t present_current[dxl_cnt_] = {0, };

  //for (int index = 0; index < dxl_cnt_; index++)
  //  present_current[index] = dxl_wb_->itemRead(dxl_id_[index], "Present_Current");

  sensor_msgs::JointState dynamixel_;
  dynamixel_.header.stamp = ros::Time::now();

  for (int index = 0; index < dxl_cnt_; index++)
  {
    std::stringstream id_num;
    id_num << "id_" << (int)(dxl_id_[index]);

    dynamixel_.name.push_back(id_num.str());

    dynamixel_.position.push_back(dxl_wb_->convertValue2Radian(dxl_id_[index], present_position[index]));
  //  dynamixel_.velocity.push_back(dxl_wb_->convertValue2Velocity(dxl_id_[index], present_velocity[index]));
  //  dynamixel_.effort.push_back(dxl_wb_->convertValue2Torque(dxl_id_[index], present_current[index]));

    present_position_[index] = dxl_wb_->convertValue2Radian(dxl_id_[index], present_position[index]);
  }

  joint_states_pub_.publish(dynamixel_);
}

void PositionControl::controlLoop()
{
  // dynamixelStatePublish();
  jointStatePublish();
}


void PositionControl::goalJointPositionCallback(const sensor_msgs::JointState::ConstPtr &msg)
{
  // double goal_position[dxl_cnt_] = {0.0, };

  for (int index = 0; index < dxl_cnt_; index++)
    goal_position[index] = msg->position.at(index);

  // int32_t goal_dxl_position[dxl_cnt_] = {0, };

  for (int index = 0; index < dxl_cnt_; index++)
  {
    goal_dxl_position[index] = dxl_wb_->convertRadian2Value(dxl_id_[index], goal_position[index]);
  }

  dxl_wb_->syncWrite("Goal_Position", goal_dxl_position);
}

int main(int argc, char **argv)
{
  // Init ROS node
  ros::init(argc, argv, "position_control");
  PositionControl pos_ctrl;

  ros::Rate loop_rate(200);

  while (ros::ok())
  {
    pos_ctrl.controlLoop();
    ros::spinOnce();
    loop_rate.sleep();
  }

  return 0;
}
