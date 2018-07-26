/**
 * @brief Multi-robot test node in a tunnel case
 */
#include "bag_writter.hpp"
#include "robot.hpp"
#include <decomp_ros_utils/data_ros_utils.h>
#include <planning_ros_utils/data_ros_utils.h>
#include <planning_ros_utils/primitive_ros_utils.h>
#include <ros/ros.h>

using namespace MPL;

vec_E<Polyhedron2D> set_obs(vec_E<Robot<2>>& robots, decimal_t time,
                            const vec_E<PolyhedronObstacle2D>& external_static_obs) {
  vec_E<Polyhedron2D> poly_obs;
  for(size_t i = 0; i < robots.size(); i++) {
    vec_E<PolyhedronLinearObstacle2D> linear_obs;
    vec_E<PolyhedronNonlinearObstacle2D> nonlinear_obs;
    for(size_t j = 0; j < robots.size(); j++) {
      if(i != j)
        nonlinear_obs.push_back(robots[j].get_nonlinear_obstacle(time));
        //linear_obs.push_back(robots[j].get_linear_obstacle(time));
    }
    robots[i].set_static_obs(external_static_obs);
    robots[i].set_linear_obs(linear_obs);
    robots[i].set_nonlinear_obs(nonlinear_obs);
    poly_obs.push_back(robots[i].get_nonlinear_obstacle(time).poly(0));
  }
  for(const auto& it: external_static_obs)
    poly_obs.push_back(it.poly(0));
  return poly_obs;
}

int main(int argc, char **argv) {
  ros::init(argc, argv, "test");
  ros::NodeHandle nh("~");

  std::string file_name;
  std::string states_name, polys_name, paths_name, prs_name;
  nh.param("file", file_name, std::string("sim.bag"));
  nh.param("states_name", states_name, std::string("/states"));
  nh.param("polys_name", polys_name, std::string("/polyhedrons"));
  nh.param("paths_name", paths_name, std::string("/paths"));
  nh.param("prs_name", prs_name, std::string("/prs"));

  ros::Publisher poly_pub =
    nh.advertise<decomp_ros_msgs::PolyhedronArray>(polys_name, 1, true);
  ros::Publisher bound_pub =
    nh.advertise<decomp_ros_msgs::PolyhedronArray>("bound", 1, true);
  ros::Publisher cloud_pub =
      nh.advertise<sensor_msgs::PointCloud>(states_name, 1, true);
  ros::Publisher prs_pub =
      nh.advertise<planning_ros_msgs::PrimitiveArray>(prs_name, 1, true);
  ros::Publisher path_pub =
      nh.advertise<planning_ros_msgs::PathArray>(paths_name, 1, true);

  Polyhedron2D rec;
  rec.add(Hyperplane2D(Vec2f(-0.5, 0), -Vec2f::UnitX()));
  rec.add(Hyperplane2D(Vec2f(0.5, 0), Vec2f::UnitX()));
  rec.add(Hyperplane2D(Vec2f(0, -0.5), -Vec2f::UnitY()));
  rec.add(Hyperplane2D(Vec2f(0, 0.5), Vec2f::UnitY()));

  Vec2f origin, dim;
  nh.param("origin_x", origin(0), 0.0);
  nh.param("origin_y", origin(1), -5.0);
  nh.param("range_x", dim(0), 10.0);
  nh.param("range_y", dim(1), 10.0);

  // Initialize planner
  double dt, v_max, a_max, t_max;
  double u;
  int num;
  nh.param("dt", dt, 1.0);
  nh.param("v_max", v_max, -1.0);
  nh.param("a_max", a_max, -1.0);
  nh.param("u", u, 1.0);
  nh.param("num", num, 1);

  // Set input control
  vec_E<VecDf> U;
  const decimal_t du = u / num;
  for (decimal_t dx = -u; dx <= u; dx += du)
    for (decimal_t dy = -u; dy <= u; dy += du)
      U.push_back(Vec2f(dx, dy));

  // Create a team of 5 robots
  Robot2D robot1(rec);
  robot1.set_v_max(v_max);
  robot1.set_a_max(a_max);
  robot1.set_u(U);
  robot1.set_dt(dt);
  robot1.set_map(origin, dim);
  robot1.set_start(Vec2f(0, -5));
  robot1.set_goal(Vec2f(10, -5));
  robot1.plan(0); // need to set a small difference in the starting time

  Robot2D robot2(rec);
  robot2.set_v_max(v_max);
  robot2.set_a_max(a_max);
  robot2.set_u(U);
  robot2.set_dt(dt);
  robot2.set_map(origin, dim);
  robot2.set_start(Vec2f(0, -2.5));
  robot2.set_goal(Vec2f(10, -2.5));
  robot2.plan(0.01); // need to set a small difference in the starting time

  Robot2D robot3(rec);
  robot3.set_v_max(v_max);
  robot3.set_a_max(a_max);
  robot3.set_u(U);
  robot3.set_dt(dt);
  robot3.set_map(origin, dim);
  robot3.set_start(Vec2f(0, 0));
  robot3.set_goal(Vec2f(10, 0));
  robot3.plan(0.02); // need to set a small difference in the starting time

  Robot2D robot4(rec);
  robot4.set_v_max(v_max);
  robot4.set_a_max(a_max);
  robot4.set_u(U);
  robot4.set_dt(dt);
  robot4.set_map(origin, dim);
  robot4.set_start(Vec2f(0, 2.5));
  robot4.set_goal(Vec2f(10, 2.5));
  robot4.plan(0.03);

  Robot2D robot5(rec);
  robot5.set_v_max(v_max);
  robot5.set_a_max(a_max);
  robot5.set_u(U);
  robot5.set_dt(dt);
  robot5.set_map(origin, dim);
  robot5.set_start(Vec2f(0, 5));
  robot5.set_goal(Vec2f(10, 5));
  robot5.plan(0.04);


  vec_E<Robot2D> robots;
  robots.push_back(robot1);
  robots.push_back(robot2);
  robots.push_back(robot3);
  robots.push_back(robot4);
  robots.push_back(robot5);

  // Build the obstacle course
  vec_E<PolyhedronObstacle2D> static_obs;

  Polyhedron2D rec2;
  rec2.add(Hyperplane2D(Vec2f(4, 0), -Vec2f::UnitX()));
  rec2.add(Hyperplane2D(Vec2f(6, 0), Vec2f::UnitX()));
  rec2.add(Hyperplane2D(Vec2f(5, 0.2), -Vec2f::UnitY()));
  rec2.add(Hyperplane2D(Vec2f(5, 5.5), Vec2f::UnitY()));
  static_obs.push_back(PolyhedronObstacle2D(rec2, Vec2f::Zero()));

  Polyhedron2D rec3;
  rec3.add(Hyperplane2D(Vec2f(4, 0), -Vec2f::UnitX()));
  rec3.add(Hyperplane2D(Vec2f(6, 0), Vec2f::UnitX()));
  rec3.add(Hyperplane2D(Vec2f(5, -5.5), -Vec2f::UnitY()));
  rec3.add(Hyperplane2D(Vec2f(5, -0.2), Vec2f::UnitY()));
  static_obs.push_back(PolyhedronObstacle2D(rec3, Vec2f::Zero()));

  vec_E<Polyhedron2D> bbox;
  bbox.push_back(robots.front().get_bbox());
  decomp_ros_msgs::PolyhedronArray bbox_msg = DecompROS::polyhedron_array_to_ros(bbox);
  bbox_msg.header.frame_id = "map";
  bbox_msg.header.stamp = ros::Time::now();
  bound_pub.publish(bbox_msg);

  // Start the replan loop
  ros::Rate loop_rate(100);
  decimal_t update_t = 0.01;
  decimal_t time = 0;
  ros::Time t0 = ros::Time::now();

  std::vector<sensor_msgs::PointCloud> cloud_msgs;
  std::vector<decomp_ros_msgs::PolyhedronArray> poly_msgs;
  std::vector<planning_ros_msgs::PathArray> path_msgs;
  std::vector<planning_ros_msgs::PrimitiveArray> prs_msgs;
  while (ros::ok()) {
    time += update_t;

    // set obstacle simultaneously
    auto poly_obs = set_obs(robots, time, static_obs); // set obstacles at time

    for(auto& it: robots)
      it.plan(time); // plan

    // Reduce the size of obstacles manually.
    for(auto& it: poly_obs) {
      for(auto& itt: it.vs_)
        itt.p_ -= itt.n_ * 0.25;
    }

    // Visualizing
    decomp_ros_msgs::PolyhedronArray poly_msg = DecompROS::polyhedron_array_to_ros(poly_obs);
    poly_msg.header.frame_id = "map";
    poly_msg.header.stamp = t0 + ros::Duration(time);
    poly_pub.publish(poly_msg);
    poly_msgs.push_back(poly_msg);

    vec_E<vec_Vec3f> path_array;
    vec_E<Primitive2D> prs_array;
    vec_Vec2f states;
    for(auto& it: robots) {
      states.push_back(it.get_state(time).pos);
      auto prs = it.get_primitives();
      prs_array.insert(prs_array.end(), prs.begin(), prs.end());
      path_array.push_back(vec2_to_vec3(it.get_history()));
    }

    auto path_msg = path_array_to_ros(path_array);
    path_msg.header.frame_id = "map";
    path_msg.header.stamp = t0 + ros::Duration(time);
    path_pub.publish(path_msg);
    path_msgs.push_back(path_msg);

    auto prs_msg = toPrimitiveArrayROSMsg(prs_array);
    prs_msg.header.frame_id = "map";
    prs_msg.header.stamp = t0 + ros::Duration(time);
    prs_pub.publish(prs_msg);
    prs_msgs.push_back(prs_msg);

    auto cloud_msg = vec_to_cloud(vec2_to_vec3(states));
    cloud_msg.header.frame_id = "map";
    cloud_msg.header.stamp = t0 + ros::Duration(time);
    cloud_pub.publish(cloud_msg);
    cloud_msgs.push_back(cloud_msg);

    ros::spinOnce();

    loop_rate.sleep();
  }

  // Write to bag (optional)
  rosbag::Bag bag;
  bag.open(file_name, rosbag::bagmode::Write);

  for(const auto& it: cloud_msgs)
    bag.write(states_name, it.header.stamp, it);
  for(const auto& it: poly_msgs)
    bag.write(polys_name, it.header.stamp, it);
  for(const auto& it: path_msgs)
    bag.write(paths_name, it.header.stamp, it);
  for(const auto& it: prs_msgs)
    bag.write(prs_name, it.header.stamp, it);

  bag.close();

  return 0;
}
