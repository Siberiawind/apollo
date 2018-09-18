/******************************************************************************
 * Copyright 2018 The Apollo Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/

#include "cybertron/proto/record.pb.h"

#include <geometry_msgs/Point32.h>
#include <geometry_msgs/Transform.h>
#include <geometry_msgs/TransformStamped.h>
#include <tf2_msgs/TFMessage.h>

#include "modules/tools/rosbag_to_record/rosbag_to_record.h"
#include "modules/tools/rosbag_to_record/channel_info.h"

#include "modules/planning/proto/planning.pb.h"
#include "modules/prediction/proto/prediction_obstacle.pb.h"
#include "modules/perception/proto/perception_obstacle.pb.h"
#include "modules/perception/proto/traffic_light_detection.pb.h"
#include "modules/canbus/proto/chassis.pb.h"
#include "modules/control/proto/control_cmd.pb.h"
#include "modules/guardian/proto/guardian.pb.h"
#include "modules/localization/proto/localization.pb.h"

using apollo::cybertron::proto::SingleMessage;
using apollo::tools::ChannelInfo;

void PrintUsage() {
  std::cout << "Usage:\n"
            << "  rosbag_to_record myfile.bag" << std::endl;
}

int main(int argc, char** argv) {
  if (argc != 2) {
    PrintUsage();
    return -1;
  }

  const std::string rosbag_file_name = argv[1];
  rosbag::Bag bag;
  try {
    bag.open(rosbag_file_name);
  } catch (...) {
    std::cerr << "Error: the input file is not a ros bag file." << std::endl;
    return -1;
  }

  auto channel_info = ChannelInfo::Instance();
  std::cout << "Info of ros bag file" << std::endl;
  std::string command_line = "rosbag info " + rosbag_file_name;
  system(command_line.c_str());

  const std::string record_file_name =
      rosbag_file_name.substr(0, rosbag_file_name.size() - 3) + "record";

  auto record_writer =
      std::make_shared<apollo::cybertron::record::RecordWriter>();
  if (!record_writer->Open(record_file_name)) {
    std::cerr << "Error: open file[" << record_file_name << "] failed.";
  }

  ros::Time::init();
  ros::Time start_time = ros::Time::now();

  rosbag::View view(bag, rosbag::TopicQuery(channel_info->GetSupportChannels()));
  
  std::vector<std::string> channel_write_flag; 
  for (const rosbag::MessageInstance m : view) {
    const std::string channel_name = m.getTopic();
    auto desc = channel_info->GetProtoDesc(channel_name);
    auto record_message_type = channel_info->GetMessageType(channel_name);
    if (desc == "" || record_message_type == "") {
      AWARN << "can not find desc or message type";
    }
    if (std::find(channel_write_flag.begin(), channel_write_flag.end(), channel_name) == channel_write_flag.end() && 
         !record_writer->WriteChannel(channel_name, record_message_type, desc)) {
      AERROR << "write channel info failed";
    } else {
      channel_write_flag.push_back(channel_name);
    }
  }

  for (const rosbag::MessageInstance m : view) {
    const std::string msg_type = m.getDataType();
    const std::string channel_name = m.getTopic();
    uint64_t nsec = m.getTime().toNSec();
    
//    auto msg_size = m.size();
//
//    std::vector<uint8_t> buffer;
//    buffer.resize(msg_size);
//    ros::serialization::IStream stream(buffer.data(), buffer.size());
//    m.write(stream);
//
//    std::string str_msg;
//    str_msg.reserve(msg_size);
//    for (size_t i = 0; i < msg_size; ++i) {
//      str_msg.push_back(buffer[i]);
//    }
    //TODO: find a way get all serialized str;
    std::string serialized_str;
    if (channel_name == "/apollo/perception/obstacles") {      
      auto pb_msg = m.instantiate<apollo::perception::PerceptionObstacles>(); 
      pb_msg->SerializeToString(&serialized_str);
    } else if (channel_name == "/apollo/planning") {
      auto pb_msg = m.instantiate<apollo::planning::ADCTrajectory>();
      pb_msg->SerializeToString(&serialized_str);
    } else if (channel_name == "/apollo/prediction") {
      auto pb_msg = m.instantiate<apollo::prediction::PredictionObstacles>();
      pb_msg->SerializeToString(&serialized_str);
    } else if (channel_name == "/apollo/canbus/chassis") {
      auto pb_msg = m.instantiate<apollo::canbus::Chassis>();
      pb_msg->SerializeToString(&serialized_str);
    } else if (channel_name == "/apollo/control") {
      auto pb_msg = m.instantiate<apollo::control::ControlCommand>();
      pb_msg->SerializeToString(&serialized_str);
    } else if (channel_name == "/apollo/guardian") {
      auto pb_msg = m.instantiate<apollo::guardian::GuardianCommand>();
      pb_msg->SerializeToString(&serialized_str);
    } else if (channel_name == "/apollo/localization/pose") {
      auto pb_msg = m.instantiate<apollo::localization::LocalizationEstimate>();
      pb_msg->SerializeToString(&serialized_str);
    } else if (channel_name == "/apollo/perception/traffic_light") {
      auto pb_msg = m.instantiate<apollo::perception::TrafficLightDetection>();
      pb_msg->SerializeToString(&serialized_str);
    } else if (channel_name == "/apollo/drive_event") {
      auto pb_msg = m.instantiate<apollo::common::DriveEvent>();
      pb_msg->SerializeToString(&serialized_str);
    } else if (channel_name == "/apollo/sensor/gnss/corrected_imu") {
      auto pb_msg = m.instantiate<apollo::localization::CorrectedImu>();
      pb_msg->SerializeToString(&serialized_str);
    } else if (channel_name == "/apollo/sensor/gnss/odometry") {
      auto pb_msg = m.instantiate<apollo::localization::Gps>();
      pb_msg->SerializeToString(&serialized_str);
    } else if (channel_name == "/apollo/monitor/system_status") {
      auto pb_msg = m.instantiate<apollo::monitor::SystemStatus>();
      pb_msg->SerializeToString(&serialized_str);
    } else if (channel_name == "/apollo/monitor/static_info") {
      auto pb_msg = m.instantiate<apollo::data::StaticInfo>();
      pb_msg->SerializeToString(&serialized_str);
    } else if (channel_name == "/apollo/monitor") {
      auto pb_msg = m.instantiate<apollo::common::monitor::MonitorMessage>();
      pb_msg->SerializeToString(&serialized_str);
    } else if (channel_name == "/apollo/canbus/chassis_detail") {
      auto pb_msg = m.instantiate<apollo::canbus::ChassisDetail>();
      pb_msg->SerializeToString(&serialized_str);
    } else if (channel_name == "/apollo/control/pad") {
      auto pb_msg = m.instantiate<apollo::control::PadMessage>();
      pb_msg->SerializeToString(&serialized_str);
    } else if (channel_name == "/apollo/navigation") {
      auto pb_msg = m.instantiate<apollo::relative_map::NavigationInfo>();
      pb_msg->SerializeToString(&serialized_str);
    } else if (channel_name == "/apollo/routing_request") {
      auto pb_msg = m.instantiate<apollo::routing::RoutingRequest>();
      pb_msg->SerializeToString(&serialized_str);
    } else if (channel_name == "/apollo/routing_response") {
      auto pb_msg = m.instantiate<apollo::routing::RoutingResponse>();
      pb_msg->SerializeToString(&serialized_str);
    } else if (channel_name == "/tf" || channel_name == "/tf_static") {
      auto rawdata = m.instantiate<tf2_msgs::TFMessage>();
      auto proto = std::make_shared<adu::common::TransformStampeds>();
      adu::common::TransformStamped *cyber_tf;
      for (size_t i = 0; i < rawdata->transforms.size(); ++i) {
        cyber_tf = proto->add_transforms();

        cyber_tf->mutable_header()->set_timestamp_sec(
            rawdata->transforms[i].header.stamp.toSec());
        cyber_tf->mutable_header()->set_frame_id(
            rawdata->transforms[i].header.frame_id);
        cyber_tf->mutable_header()->set_sequence_num(
            rawdata->transforms[i].header.seq);

        cyber_tf->set_child_frame_id(rawdata->transforms[i].child_frame_id);

        cyber_tf->mutable_transform()->mutable_translation()->set_x(
            rawdata->transforms[i].transform.translation.x);
        cyber_tf->mutable_transform()->mutable_translation()->set_y(
            rawdata->transforms[i].transform.translation.y);
        cyber_tf->mutable_transform()->mutable_translation()->set_z(
            rawdata->transforms[i].transform.translation.z);
        cyber_tf->mutable_transform()->mutable_rotation()->set_qx(
            rawdata->transforms[i].transform.rotation.x);
        cyber_tf->mutable_transform()->mutable_rotation()->set_qy(
            rawdata->transforms[i].transform.rotation.y);
        cyber_tf->mutable_transform()->mutable_rotation()->set_qz(
            rawdata->transforms[i].transform.rotation.z);
        cyber_tf->mutable_transform()->mutable_rotation()->set_qw(
            rawdata->transforms[i].transform.rotation.w);
      }
      proto->SerializeToString(&serialized_str);
    } else if (channel_name == "/apollo/sensor/conti_radar") {
      auto pb_msg = m.instantiate<apollo::drivers::ContiRadar>();
      pb_msg->SerializeToString(&serialized_str);
    } else if (channel_name == "/apollo/sensor/delphi_esr") {
      auto pb_msg = m.instantiate<apollo::drivers::DelphiESR>();
      pb_msg->SerializeToString(&serialized_str);
    } else if (channel_name == "/apollo/sensor/gnss/best_pose") {
      auto pb_msg = m.instantiate<apollo::drivers::gnss::GnssBestPose>();
      pb_msg->SerializeToString(&serialized_str);
    } else if (channel_name == "/apollo/sensor/gnss/gnss_status") {
      auto pb_msg = m.instantiate<apollo::drivers::gnss_status::GnssStatus>();
      pb_msg->SerializeToString(&serialized_str);
    } else if (channel_name == "/apollo/sensor/gnss/imu") {
      auto pb_msg = m.instantiate<apollo::drivers::gnss::Imu>();
      pb_msg->SerializeToString(&serialized_str);
    } else if (channel_name == "/apollo/sensor/gnss/ins_stat") {
      auto pb_msg = m.instantiate<apollo::drivers::gnss::InsStat>();
      pb_msg->SerializeToString(&serialized_str);
    } else if (channel_name == "/apollo/sensor/gnss/rtk_eph") {
      auto pb_msg = m.instantiate<apollo::drivers::gnss::GnssEphemeris>();
      pb_msg->SerializeToString(&serialized_str);
    } else if (channel_name == "/apollo/sensor/gnss/rtk_obs") {
      auto pb_msg = m.instantiate<apollo::drivers::gnss::EpochObservation>();
      pb_msg->SerializeToString(&serialized_str);
    } else {
      AWARN << "not support channel:" << channel_name;
      continue;
    }
 
//    auto desc = channel_info->GetProtoDesc(channel_name);
//    auto record_message_type = channel_info->GetMessageType(channel_name);
//    if (desc == "" || record_message_type == "") {
//      AWARN << "can not find desc or message type";
//    }
//    if (!record_writer->WriteChannel(channel_name, record_message_type, desc)) {
//      AERROR << "write channel info failed";
//    }
//    SingleMessage single_msg;
//    single_msg.set_channel_name(channel_name);
//    
//    single_msg.set_content(serialized_str);
//    single_msg.set_time(nsec);
    auto raw_message = 
        std::make_shared<apollo::cybertron::message::RawMessage>(serialized_str);
    if (!record_writer->WriteMessage(channel_name, raw_message, nsec)) {
      AERROR << "write single msg fail";
    }
  }

  record_writer->Close();
  record_writer = nullptr;
  std::cout << "Info of record file" << std::endl;
  command_line = "cyber_recorder info -f " + record_file_name;
  system(command_line.c_str());

  std::cout << "Convertion finished! Took " << ros::Time::now() - start_time
            << " seconds in total." << std::endl;
  return 0;
}
