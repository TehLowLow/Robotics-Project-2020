#include "ros/ros.h"
#include "std_msgs/String.h"
#include <iostream>
#include <nav_msgs/Odometry.h>
#include <sensor_msgs/NavSatFix.h>
#include <tf/transform_broadcaster.h>
#include <string.h>




#define BUFFER_SIZE 50



struct encoded {

    float No;
    float Ea;
    float U;

};

static struct encoded *vehicle;

using std::string;

string tfObject;



/**
 * variabile di tipo custom message
 */
nav_msgs::Odometry vehicleEncodedMessage;
ros::Publisher encodedTopic;


//fixed position of the car
float latitude_init;
float longitude_init;
float h0;

static struct encoded *lla2enu(const sensor_msgs::NavSatFix::ConstPtr &msg);

void topicManager(const sensor_msgs::NavSatFix_<std::allocator<void>>::ConstPtr &msg);


/**
 * Conversion method
 * Used to convert project.bag data from lla to ENU standard
 * @param msg is the message coming from topic
 * @return a struct containing required data
 */


struct encoded *lla2enu(const sensor_msgs::NavSatFix_<std::allocator<void>>::ConstPtr &msg) {

    //ROS_INFO("Input position: [%f,%f, %f]", msg->latitude, msg->longitude,msg->altitude);
    // fixed values

    double a = 6378137;
    double b = 6356752.3142;
    double f = (a - b) / a;
    double e_sq = f * (2 - f);
    float deg_to_rad = 0.0174533;

    // input data from msg
    float latitude = msg->latitude;
    float longitude = msg->longitude;
    float h = msg->altitude;

    //lla to ecef
    float lamb = deg_to_rad * (latitude);
    float phi = deg_to_rad * (longitude);
    float s = sin(lamb);
    float North = a / sqrt(1 - e_sq * s * s);

    float sin_lambda = sin(lamb);
    float cos_lambda = cos(lamb);
    float sin_phi = sin(phi);
    float cos_phi = cos(phi);

    float x = (h + North) * cos_lambda * cos_phi;
    float y = (h + North) * cos_lambda * sin_phi;
    float z = float(h + (1 - e_sq) * North) * sin_lambda;

    //ROS_INFO("ECEF position: [%f,%f, %f]", x, y,z);


    // ecef to enu

    lamb = deg_to_rad * (latitude_init);
    phi = deg_to_rad * (longitude_init);
    s = sin(lamb);
    North = a / sqrt(1 - e_sq * s * s);

    sin_lambda = sin(lamb);
    cos_lambda = cos(lamb);
    sin_phi = sin(phi);
    cos_phi = cos(phi);

    float x0 = (h0 + North) * cos_lambda * cos_phi;
    float y0 = (h0 + North) * cos_lambda * sin_phi;
    float z0 = float(h0 + (1 - e_sq) * North) * sin_lambda;

    float xd = x - x0;
    float yd = y - y0;
    float zd = z - z0;

    struct encoded *temp = new struct encoded;


    temp->Ea = -sin_phi * xd + cos_phi * yd;
    temp->No = -cos_phi * sin_lambda * xd - sin_lambda * sin_phi * yd + cos_lambda * zd;
    temp->U = cos_lambda * cos_phi * xd + cos_lambda * sin_phi * yd + sin_lambda * zd;

    //ROS_INFO("ENU position: [%f,%f, %f]", temp->Ea, temp->No,temp->U);

    return temp;
}

/**
 * This function is used to manage the conversion from lla2enu from topics
 * @param msg
 */
void topicManager(const sensor_msgs::NavSatFix_<std::allocator<void>>::ConstPtr &msg) {

    if (msg->latitude != 0 && msg->longitude != 0 && msg->altitude != 0) {

        vehicle = lla2enu(msg);
        vehicleEncodedMessage.pose.pose.position.x = vehicle->Ea;
        vehicleEncodedMessage.pose.pose.position.y = vehicle->No;
        vehicleEncodedMessage.pose.pose.position.z = vehicle->U;


    } else {

        vehicleEncodedMessage.pose.pose.position.x = 0;
        vehicleEncodedMessage.pose.pose.position.y = 0;
        vehicleEncodedMessage.pose.pose.position.z = 0;
    }


    //ROS_INFO("Leggo i dati di custom msg: %f , %f , %f ",vehicleEncodedMessage.E, vehicleEncodedMessage.N, vehicleEncodedMessage.Up);


    encodedTopic.publish(vehicleEncodedMessage);


}


/**
* Conversion to ENU
* @param msg gps message
*/

int main(int argc, char **argv) {

    ROS_INFO("argc: %d  argv: %s %s", argc, argv[0], argv[1]);

    ros::init(argc, argv, "ros_proj");

    latitude_init = atof(argv[1]);
    longitude_init = atof(argv[2]);
    h0 = atof(argv[3]);


    ros::NodeHandle initNode;

    ros::Subscriber bagTopic = initNode.subscribe(argv[4], BUFFER_SIZE, topicManager);
    tfObject = argv[5];
    encodedTopic = initNode.advertise<nav_msgs::Odometry>(tfObject, BUFFER_SIZE);


    ros::spin();

    return 0;

}

/*TODO cercare qualsiasi valore hardcoddato che scasserebbe il codice se cambiato nel launcher
       come ad esempio in master cpp carSub e obsSub @attention
*/
