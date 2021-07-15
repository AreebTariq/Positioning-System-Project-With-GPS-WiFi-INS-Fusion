#include <iostream>
#include<stdio.h>
#include<string>
#include<math.h>

using namespace std;

#define VALUE_LEN 30
#define SENSOR_NAME_LEN 10
#define TOTAL_READINGS 2000
#define ACC_SAMPLING_FREQ 5  //should be greater or equal to 1
#define GPS_SAMPLING_FREQ 1  //should be greater or equal to 1
#define MAG_SAMPLING_FREQ 1  //should be greater or equal to 1
#define GYR_SAMPLING_FREQ 1  //should be greater or equal to 1
#define FOOTSTEP_SIZE 0.6
#define M_PI (3.14)
#define GPS_TIME_TRANSITION_MS 1200
#define INVALID_VALUE 0xFFFFF
#define GPS_ACC_TRANSITION_THRESHOLD -1
#define GYRO_LOWER_THRESHOLD -0.8
#define GYRO_UPPER_THRESHOLD 0.8
#define GYRO_CLOCKWISE_ADJ_FAC 1.1
#define GYRO_COUNTER_CLOCKWISE_ADJ_FAC 1.4
#define MAG_THRESHOLD 1.5
#define WIFI_POWER_THRESHOLD -50

#define NO_ANGULAR_MOVEMENT 0
#define CLOCKWISE_ANGULAR_MOVEMENT 1
#define COUNTER_CLOCKWISE_ANGULAR_MOVEMENT 2

enum USER_STATE
{
	OUTDOOR,
	TRANSITION,
	INDOOR,
	INDOOR_NO_WIFI
};
typedef struct acc_data
{
	float timestamp;
	char sensor_name[SENSOR_NAME_LEN];
	float acc_x;
	float acc_y;
	float acc_z;
};

typedef struct mag_data
{
	float timestamp;
	char sensor_name[SENSOR_NAME_LEN];
	float x;
	float y;
	float z;
};

typedef struct gyro_data
{
	float timestamp;
	char sensor_name[SENSOR_NAME_LEN];
	float x;
	float y;
	float z;
};

typedef struct gps_data
{
	float timestamp;
	char sensor_name[SENSOR_NAME_LEN];
	float longitude;
	float latitude;
	float altitude;
};

typedef struct wifi_data
{
	float timestamp;
	char sensor_name[SENSOR_NAME_LEN];
	char mac[VALUE_LEN];
	char wifi_name [VALUE_LEN];
	float wifi_power;
};

typedef struct distance_info
{
	float distance;
	float timestamp;
};

typedef struct direction_info
{
	int direction;
	float timestamp;
};

typedef struct angle_info
{
	float angle;
	float timestamp;
};

int total_acc_readings = 0;
int total_gps_readings = 0;
int total_mag_readings = 0;
int total_gyro_readings = 0;
int total_fused_readings = 0;
int total_wifi_readings = 0;
float total_mag_blocks = 0;
char home_wifi_mac[] = "d8:84:66:8f:ae:99";

char filename[] = "F:\\Docs\\2-MS\\My MS\\France\\1- Academics\\Courses\\Spring 2021\\Radio Frequency\\Project 2\\Demo\\150m_straight_outdoor.txt";

float toRadians(float degree)
{
	float one_deg = (M_PI) / 180;
	return (one_deg * degree);
}

void write_processed_wifi_data_to_file(wifi_data* readings)
{
	char power[VALUE_LEN] = "test";
	char time[VALUE_LEN] = "test";
	
	// open the file for writing
	FILE* fp = fopen("C:\\Users\\Areeb\\OneDrive\\Desktop\\Sensor_Fusion_Data\\Wifi_data.csv", "w+");

	if (fp == NULL)
	{
		//	printf("Error opening the file");
		return;
	}

	for (int i = 0; i < total_wifi_readings;i++)
	{
		_gcvt(readings[i].wifi_power, 7, power);
		_gcvt(readings[i].timestamp, 7, time);

		fprintf(fp, "%s,%s,%s\n", readings[i].wifi_name, readings[i].mac, power);
	}

	// close the file
	fclose(fp);
}

void read_wifi_data_from_file(wifi_data * readings)
{
	FILE* in = fopen(filename, "r");
	char* words[100];
	int word_count = 0;

	const unsigned MAX_LENGTH = 256;
	char buffer[MAX_LENGTH];
	char* command_token;
	int  command_token_number = 0;

	char *mac_address;
	char* wifi_name;
	float wifi_power;

	float last_time;

	while (fgets(buffer, MAX_LENGTH, in))
	{
		//printf("%s", buffer);

		/*Remove the new line character at the end*/
		strtok(buffer, "\n");

		command_token = strtok(buffer, "\t");

		while (command_token != NULL)
		{
			words[command_token_number++] = command_token;
			command_token = strtok(NULL, "\t");
		}

		words[command_token_number] = NULL;
		command_token_number = 0;

		if (strcmp(words[1], "RSSWIFI") == 0)
		{
			if (strcmp(words[2], home_wifi_mac) == 0)
			{
				//printf("\nHOME MAC FOUND");
				strcpy(readings[total_wifi_readings].mac, words[2]);
				strcpy(readings[total_wifi_readings].wifi_name, words[3]);
				readings[total_wifi_readings].wifi_power = (float)atof(words[5]);
				readings[total_wifi_readings].timestamp = last_time;
				total_wifi_readings++;
			}
		}
		else 
		{
			last_time = (float)atof(words[0]);
		}
	}
	fclose(in);
}

void write_processed_acc_data_to_file(acc_data *readings)
{
	char acc_x[VALUE_LEN] = "test";
	char acc_y[VALUE_LEN] = "test";
	char acc_z[VALUE_LEN] = "test";
	char timestamp[VALUE_LEN] = "test";

	// open the file for writing
	FILE* fp = fopen("C:\\Users\\Areeb\\OneDrive\\Desktop\\Sensor_Fusion_Data\\Acc_data.csv", "w+");
	
	if (fp == NULL)
	{
	//	printf("Error opening the file");
		return ;
	}

	for (int i = 0; i < total_acc_readings;i++)
	{
		_gcvt(readings[i].acc_x, 7, acc_x);
		_gcvt(readings[i].acc_y, 7, acc_y);
		_gcvt(readings[i].acc_z, 7, acc_z);
		_gcvt(readings[i].timestamp, 10, timestamp);
		fprintf(fp, "%s,%s,%s,%s\n", timestamp, acc_x, acc_y, acc_z);
	}

	// close the file
	fclose(fp);
}

void read_acc_data_from_file(acc_data* readings)
{
	FILE* in = fopen(filename, "r");
	char* words[100];
	int word_count = 0;
	
	const unsigned MAX_LENGTH = 256;
	char buffer[MAX_LENGTH];
	char* command_token;
	int  command_token_number = 0;
	
	float average_acc_x = 0;
	float average_acc_y = 0;
	float average_acc_z = 0;
	float average_timestamp = 0;
	int avaerage_counter = 0;

	while (fgets(buffer, MAX_LENGTH, in))
	{
		//printf("%s", buffer);

		/*Remove the new line character at the end*/
		strtok(buffer, "\n");

		command_token = strtok(buffer, "\t");

		while (command_token != NULL)
		{
			words[command_token_number++] = command_token;
			command_token = strtok(NULL, "\t");
		}

		words[command_token_number] = NULL;
		command_token_number = 0;

		if (strcmp(words[1], "ACC") == 0)
		{
			average_acc_x = average_acc_x + (float)atof(words[2]);
			average_acc_y = average_acc_y + (float)atof(words[3]);
			average_acc_z = average_acc_z + (float)atof(words[4]);
			average_timestamp = average_timestamp + (float)atof(words[0]);
			
			avaerage_counter++;
		}

		if (avaerage_counter == ACC_SAMPLING_FREQ)
		{
			readings[total_acc_readings].acc_x = average_acc_x/ ACC_SAMPLING_FREQ;
			readings[total_acc_readings].acc_y = average_acc_y/ ACC_SAMPLING_FREQ;
			readings[total_acc_readings].acc_z= average_acc_z/ ACC_SAMPLING_FREQ;
			readings[total_acc_readings].timestamp = average_timestamp/ ACC_SAMPLING_FREQ;
			total_acc_readings++;

			average_acc_x = 0;
			average_acc_y = 0;
			average_acc_z = 0;
			average_timestamp = 0;
			avaerage_counter = 0;
		}	
	}
	fclose(in);
}

void write_processed_gps_data_to_file(gps_data* readings)
{
	char longi[VALUE_LEN] = "test";
	char lat[VALUE_LEN] = "test";
	char alt[VALUE_LEN] = "test";
	char timestamp[VALUE_LEN] = "test";

	// open the file for writing
	FILE* fp = fopen("C:\\Users\\Areeb\\OneDrive\\Desktop\\Sensor_Fusion_Data\\Gps_data.csv", "w+");

	if (fp == NULL)
	{
		//printf("Error opening the file");
		return;
	}

	for (int i = 0; i < total_gps_readings;i++)
	{
		_gcvt(readings[i].longitude, 7, longi);
		_gcvt(readings[i].latitude, 7, lat);
		_gcvt(readings[i].altitude, 7, alt);
		_gcvt(readings[i].timestamp, 10, timestamp);
		fprintf(fp, "%s,%s,%s,%s\n", timestamp, longi, lat, alt);
	}

	// close the file
	fclose(fp);
}

void read_gps_data_from_file(gps_data* readings)
{
	FILE* in = fopen(filename, "r");
	char* words[100];
	int word_count = 0;

	const unsigned MAX_LENGTH = 256;
	char buffer[MAX_LENGTH];
	char* command_token;
	int  command_token_number = 0;

	float average_long = 0;
	float average_lat = 0;
	float average_alt = 0;
	float average_timestamp = 0;
	int avaerage_counter = 0;

	while (fgets(buffer, MAX_LENGTH, in))
	{
		//printf("%s", buffer);

		/*Remove the new line character at the end*/
		strtok(buffer, "\n");

		command_token = strtok(buffer, "\t");

		while (command_token != NULL)
		{
			words[command_token_number++] = command_token;
			command_token = strtok(NULL, "\t");
		}

		words[command_token_number] = NULL;
		command_token_number = 0;

		if (strcmp(words[1], "GPS") == 0)
		{
			average_lat = average_lat + (float)atof(words[2]);
			average_long = average_long + (float)atof(words[3]);
			average_alt = average_alt + (float)atof(words[4]);
			average_timestamp = average_timestamp + (float)atof(words[0]);

			avaerage_counter++;
		}

		if (avaerage_counter == GPS_SAMPLING_FREQ)
		{
			readings[total_gps_readings].longitude = average_long / GPS_SAMPLING_FREQ;
			readings[total_gps_readings].latitude = average_lat / GPS_SAMPLING_FREQ;
			readings[total_gps_readings].altitude = average_alt / GPS_SAMPLING_FREQ;
			readings[total_gps_readings].timestamp = average_timestamp / GPS_SAMPLING_FREQ;
			total_gps_readings++;

			average_long = 0;
			average_lat = 0;
			average_alt = 0;
			average_timestamp = 0;
			avaerage_counter = 0;
		}
	}
	fclose(in);
}

void write_processed_mag_data_to_file(mag_data* readings)
{
	char x[VALUE_LEN] = "test";
	char y[VALUE_LEN] = "test";
	char z[VALUE_LEN] = "test";
	char timestamp[VALUE_LEN] = "test";

	// open the file for writing
	FILE* fp = fopen("C:\\Users\\Areeb\\OneDrive\\Desktop\\Sensor_Fusion_Data\\Mag_data.csv", "w+");

	if (fp == NULL)
	{
		//printf("Error opening the file");
		return;
	}

	for (int i = 0; i < total_mag_readings;i++)
	{
		_gcvt(readings[i].x, 7, x);
		_gcvt(readings[i].y, 7, y);
		_gcvt(readings[i].z, 7, z);
		_gcvt(readings[i].timestamp, 10, timestamp);
		fprintf(fp, "%s,%s,%s,%s\n", timestamp, x, y, z);
	}

	// close the file
	fclose(fp);
}

void read_mag_data_from_file(mag_data* readings)
{
	FILE* in = fopen(filename, "r");
	char* words[100];
	int word_count = 0;

	const unsigned MAX_LENGTH = 256;
	char buffer[MAX_LENGTH];
	char* command_token;
	int  command_token_number = 0;

	float average_long = 0;
	float average_lat = 0;
	float average_alt = 0;
	float average_timestamp = 0;
	int avaerage_counter = 0;

	while (fgets(buffer, MAX_LENGTH, in))
	{
		//printf("%s", buffer);

		/*Remove the new line character at the end*/
		strtok(buffer, "\n");

		command_token = strtok(buffer, "\t");

		while (command_token != NULL)
		{
			words[command_token_number++] = command_token;
			command_token = strtok(NULL, "\t");
		}

		words[command_token_number] = NULL;
		command_token_number = 0;

		if (strcmp(words[1], "MAG") == 0)
		{
			average_long = average_long + (float)atof(words[2]);
			average_lat = average_lat + (float)atof(words[3]);
			average_alt = average_alt + (float)atof(words[4]);
			average_timestamp = average_timestamp + (float)atof(words[0]);

			avaerage_counter++;
		}

		if (avaerage_counter == MAG_SAMPLING_FREQ)
		{
			readings[total_mag_readings].x = average_long / MAG_SAMPLING_FREQ;
			readings[total_mag_readings].y = average_lat / MAG_SAMPLING_FREQ;
			readings[total_mag_readings].z = average_alt / MAG_SAMPLING_FREQ;
			readings[total_mag_readings].timestamp = average_timestamp / MAG_SAMPLING_FREQ;
			total_mag_readings++;

			average_long = 0;
			average_lat = 0;
			average_alt = 0;
			average_timestamp = 0;
			avaerage_counter = 0;
		}
	}
	fclose(in);
}

void write_processed_gyro_data_to_file(gyro_data* readings)
{
	char x[VALUE_LEN] = "test";
	char y[VALUE_LEN] = "test";
	char z[VALUE_LEN] = "test";
	char timestamp[VALUE_LEN] = "test";

	// open the file for writing
	FILE* fp = fopen("C:\\Users\\Areeb\\OneDrive\\Desktop\\Sensor_Fusion_Data\\Gyr_data.csv", "w+");

	if (fp == NULL)
	{
		//printf("Error opening the file");
		return;
	}

	for (int i = 0; i < total_gyro_readings;i++)
	{
		_gcvt(readings[i].x, 7, x);
		_gcvt(readings[i].y, 7, y);
		_gcvt(readings[i].z, 7, z);
		_gcvt(readings[i].timestamp, 10, timestamp);
		fprintf(fp, "%s,%s,%s,%s\n", timestamp, x, y, z);
	}

	// close the file
	fclose(fp);
}

void read_gyro_data_from_file(gyro_data* readings)
{
	FILE* in = fopen(filename, "r");
	char* words[100];
	int word_count = 0;

	const unsigned MAX_LENGTH = 256;
	char buffer[MAX_LENGTH];
	char* command_token;
	int  command_token_number = 0;

	float average_long = 0;
	float average_lat = 0;
	float average_alt = 0;
	float average_timestamp = 0;
	int avaerage_counter = 0;

	while (fgets(buffer, MAX_LENGTH, in))
	{
		//printf("%s", buffer);

		/*Remove the new line character at the end*/
		strtok(buffer, "\n");

		command_token = strtok(buffer, "\t");

		while (command_token != NULL)
		{
			words[command_token_number++] = command_token;
			command_token = strtok(NULL, "\t");
		}

		words[command_token_number] = NULL;
		command_token_number = 0;

		if (strcmp(words[1], "GYR") == 0)
		{
			average_long = average_long + (float)atof(words[2]);
			average_lat = average_lat + (float)atof(words[3]);
			average_alt = average_alt + (float)atof(words[4]);
			average_timestamp = average_timestamp + (float)atof(words[0]);

			avaerage_counter++;
		}

		if (avaerage_counter == GYR_SAMPLING_FREQ)
		{
			readings[total_gyro_readings].x = average_long / GYR_SAMPLING_FREQ;
			readings[total_gyro_readings].y = average_lat / GYR_SAMPLING_FREQ;
			readings[total_gyro_readings].z = average_alt / GYR_SAMPLING_FREQ;
			readings[total_gyro_readings].timestamp = average_timestamp / GYR_SAMPLING_FREQ;
			total_gyro_readings++;

			average_long = 0;
			average_lat = 0;
			average_alt = 0;
			average_timestamp = 0;
			avaerage_counter = 0;
		}
	}
	fclose(in);
}

void scale_time_of_measurements(acc_data* acc_values, mag_data* mag_values, gps_data* gps_values, gyro_data *gyro_values, wifi_data *wifi_values)
{
	float reference_time = acc_values[0].timestamp;

	for (int i = 0;i < total_acc_readings;i++)
	{
		acc_values[i].timestamp -= reference_time;
	}

	for (int i = 0;i < total_mag_readings;i++)
	{
		mag_values[i].timestamp -= reference_time;
	}

	for (int i = 0;i < total_gps_readings;i++)
	{
		gps_values[i].timestamp -= reference_time;
	}

	for (int i = 0;i < total_gyro_readings;i++)
	{
		gyro_values[i].timestamp -= reference_time;
	}

	for (int i = 0; i < total_wifi_readings; i++)
	{
		wifi_values[i].timestamp -= reference_time;
	}
}

int  get_corresponding_direction(direction_info* direction_values, float reference_time)
{
	float min_dif = 999999, temp;
	int min_idx = INVALID_VALUE;

	for (int i = 0; i < total_gyro_readings;i++)
	{
		temp = fabs(direction_values[i].timestamp - reference_time);
		if (temp < min_dif)
		{
			min_dif = temp;
			min_idx = i;
		}
	}

	return direction_values[min_idx].direction;
}

void find_angles_with_mag(mag_data *mag_data, angle_info *angles, direction_info *direction_values, float initial_time)
{
	angles[0].angle = 90;
	angles[0].timestamp = mag_data[0].timestamp;
	float temp ,diff_angle =0;
	int direction;

	for (int i = 1; i < total_mag_readings;i++)
	{
		temp = (mag_data[i].y - mag_data[i - 1].y);
		direction = get_corresponding_direction(direction_values, mag_data[i].timestamp);
		if (fabs(temp) < MAG_THRESHOLD && direction == NO_ANGULAR_MOVEMENT)
		{
			diff_angle = 0;
		}
		else
		{
			diff_angle = fabs(temp) * 5;
		}
		

		if (direction == COUNTER_CLOCKWISE_ANGULAR_MOVEMENT)
		{
			angles[i].angle = angles[i - 1].angle + (GYRO_COUNTER_CLOCKWISE_ADJ_FAC *diff_angle);
		}
		else if (direction == CLOCKWISE_ANGULAR_MOVEMENT)
		{
			angles[i].angle = angles[i - 1].angle - (GYRO_CLOCKWISE_ADJ_FAC * diff_angle);
		}
		else
		{
			angles[i].angle = angles[i - 1].angle;
		}

		angles[i].timestamp = mag_data[i].timestamp;

		//printf("\nMAG data mapped angle in degrees from reference horizontal axis at time=%fms is =%f", angles[i].timestamp, angles[i].angle);
	}
}

void find_direction_with_gyro(gyro_data* gyro_data, direction_info* direction)
{
	float temp;
	for (int i = 0; i < total_gyro_readings;i++)
	{
		if (gyro_data[i].z < GYRO_LOWER_THRESHOLD)
		{
			direction[i].direction = CLOCKWISE_ANGULAR_MOVEMENT;
			//printf("\nCOUNTERCLOCKWISE at time=%fms", gyro_data[i].timestamp);
		}
		else if (gyro_data[i].z > GYRO_UPPER_THRESHOLD)
		{
			direction[i].direction = COUNTER_CLOCKWISE_ANGULAR_MOVEMENT;
			//printf("\nCLOCKWISE at time=%fms", gyro_data[i].timestamp);

		}
		else
		{
			direction[i].direction = NO_ANGULAR_MOVEMENT;
		}

		direction[i].timestamp = gyro_data[i].timestamp;
		//printf("\nGYRO data mapped angular direction at t=%fms =%d", direction[i].timestamp, direction[i].direction);
	}
}

float  map_distance_to_angle(angle_info* angle_values, float reference_time)
{
	float min_dif = 999999, temp;
	int min_idx = INVALID_VALUE;
	float angle = 0;

	if (reference_time < angle_values[0].timestamp)
	{
		return angle_values[0].angle;
	}

	for (int i = 0; i < total_mag_readings;i++)
	{
		temp = fabs(angle_values[i].timestamp - reference_time);
		if (temp < min_dif)
		{
			min_dif = temp;
			min_idx = i;
		}
	}

	return angle_values[min_idx].angle;
}

void find_xy_coordinates(direction_info *direction_values, distance_info *distance_array, angle_info *angles, float *x, float *y)
{
	int range;
	float angle;
	int direction;
	char x_val[30] = "test";
	char y_val[30] = "test";

	if (total_fused_readings > total_mag_readings)
	{
		if (total_mag_readings > total_gyro_readings)
		{
			range = total_gyro_readings;
		}
		else
		{
			range = total_mag_readings;
		}
		
	}
	else
	{
		range = total_fused_readings;
	}
	x[0] = 0;
	y[0] = 0;
	for (int i = 1;i < range;i++)
	{		
		direction = get_corresponding_direction(direction_values, distance_array[i].timestamp);
		angle = map_distance_to_angle(angles, distance_array[i].timestamp);

		//printf("\nangle = %f", angle);
		x[i] = x[i-1]+ (distance_array[i].distance)* cos(0.017* (angle));
		y[i] = y[i-1]+ (distance_array[i].distance) * sin(0.017 * (angle));

		//printf("\n x =%fm \t y=%fm", x[i], y[i]);
	}

	FILE* fp = fopen("C:\\Users\\Areeb\\OneDrive\\Desktop\\Sensor_Fusion_Data\\XY_data.csv", "w+");

	if (fp == NULL)
	{
		//printf("Error opening the file");
		return;
	}

	for (int i = 0; i < range;i++)
	{
		_gcvt(x[i], 7, x_val);
		_gcvt(y[i], 7, y_val);
		fprintf(fp, "%s,%s\n", x_val,y_val);
	}

	// close the file
	fclose(fp);
}

void find_distance_from_acc(acc_data* values, distance_info* distance_array)
{
	float mean_acc = 0;
	int zero_crossings = 0;
	int arr[1000];
	for (int i = 0; i < total_acc_readings;i++)
	{
		mean_acc = mean_acc + values[i].acc_y;
	}

	mean_acc = mean_acc / total_acc_readings;

	for (int i = 1; i < total_acc_readings;i++)
	{
		float temp1, temp2;
		temp1 = values[i].acc_y - mean_acc;
		temp2 = values[i - 1].acc_y - mean_acc;

		if (signbit(temp1) != signbit(temp2))
		{
			arr[zero_crossings] = i;
			zero_crossings++;
		}
	}

	distance_array[0].distance = 0;
	distance_array[0].timestamp = values[0].timestamp;

	int zc_index = 0;
	for (int i = 1;i < total_acc_readings;i++)
	{
		float temp3;

		if (arr[zc_index] == i)
		{
			distance_array[i].distance = +0.3;
			zc_index++;
		}
		else
		{
			distance_array[i].distance = 0;
		}

		distance_array[i].timestamp = values[i].timestamp;

		//printf("\nAccelerometer incremental distance --> t =%f,   d= %f", distance_array[i].timestamp, distance_array[i].distance);
	}

	//printf("\nMean Acc_val = %f, Number of mean (zero) crossings = %d", mean_acc, zero_crossings);
	//printf("\nTotal predicted distance based on accelerometer=%f", (zero_crossings / 2) * FOOTSTEP_SIZE);

}

void find_distance_with_gps(gps_data *gps_values, distance_info *gps_distance_array)
{
	gps_distance_array[0].distance = 0;
	gps_distance_array[0].timestamp = gps_values[0].timestamp;

	float lat1, lang1, lat2, lang2, dlong, dlat, ans;
	float R = 6731000; //radius of earth

	for (int i = 1;i < total_gps_readings;i++)
	{
		lat1 = toRadians(gps_values[i-1].latitude);
		lang1 = toRadians(gps_values[i-1].longitude);
		lat2 = toRadians(gps_values[i].latitude);
		lang2 = toRadians(gps_values[i].longitude);

		 dlong = lang2 - lang1;
		dlat = lat2 - lat1;

		ans = pow(sin(dlat / 2), 2) +
			cos(lat1) * cos(lat2) *
			pow(sin(dlong / 2), 2);

		ans = 2 * asin(sqrt(ans));

		// Calculate the result
		ans = ans * R;

		gps_distance_array[i].distance = ans;
		gps_distance_array[i].timestamp = gps_values[i].timestamp;

		//printf("\nGPS incremental distance --> t =%f,   d= %f", gps_distance_array[i].timestamp, gps_distance_array[i].distance);
	}
}

float interpolate_distance(float time, float distance, float ref_time)
{
	float interpolated_distance;

	interpolated_distance = (ref_time / time) * distance;

	return interpolated_distance;
}

float find_gps_step_time(float initial_time, distance_info *data)
{
	float min_diff = 999999, temp_diff;
	int idx;
	float step_time = INVALID_VALUE;;
	float temp_time = data[0].timestamp;
	int gps_right_end_idx = 0;

	for (int i = 0; i < total_gps_readings; i++)
	{
		temp_diff = data[i].timestamp -initial_time;
		if (temp_diff >0)
		{
			step_time = temp_diff;
			break;
		}
	}
	return step_time;
}

float find_acc_step_time(float initial_time, distance_info* data)
{
	float min_diff = 999999, temp_diff;
	int idx;
	float step_time = INVALID_VALUE;;
	float temp_time = data[0].timestamp;
	int gps_right_end_idx = 0;

	for (int i = 0; i < total_acc_readings; i++)
	{
		temp_diff = data[i].timestamp - initial_time;
		if (temp_diff > 0)
		{
			step_time = temp_diff;
			break;
		}
	}
	return step_time;
}

float find_step_distance(float right_end_time, float left_end_time, distance_info *data)
{
	int idx = 0;
	float right_distance = 0, interpolated_distance =0, left_distance =0;
	float time_diff, distance_diff;
	
	while (data[idx].timestamp < right_end_time)
	{
		right_distance += data[idx].distance;
		idx++;
	}
	if (idx > 0)
	{
		time_diff = data[idx].timestamp - data[idx-1].timestamp;
		time_diff = data[idx].distance / time_diff;
		distance_diff = time_diff * (right_end_time - data[idx - 1].timestamp);
		right_distance += distance_diff;
	}

	idx = 0;
	interpolated_distance = 0;
	time_diff = 0;
	distance_diff = 0;

	while (data[idx].timestamp < left_end_time)
	{
		left_distance += data[idx].distance;
		idx++;
	}
	if (idx > 0)
	{
		time_diff = data[idx].timestamp - data[idx - 1].timestamp;
		time_diff = data[idx].distance / time_diff;
		distance_diff = time_diff * (left_end_time - data[idx - 1].timestamp);
		left_distance += distance_diff;
	}

	return left_distance - right_distance;
}

void find_distance_with_fusion(distance_info *acc_distance_array, distance_info* gps_distance_array, distance_info* fusion_data, wifi_data *wifi_data)
{
	float min_time, max_time, gps_diff,acc_diff, gps_distance,acc_distance;
	int gps_idx = 1, acc_idx = 0;
	float temp1 = 0;
	int wifi_index = 0;

	if (acc_distance_array[0].timestamp < gps_distance_array[0].timestamp || gps_distance_array[0].timestamp < 0)
	{
		min_time = acc_distance_array[0].timestamp;
	}
	else
	{
		min_time = gps_distance_array[0].timestamp;
	}

	if (acc_distance_array[total_acc_readings-1].timestamp < gps_distance_array[total_gps_readings-1].timestamp)
	{
		max_time = gps_distance_array[total_gps_readings-1].timestamp;
	}
	else
	{
		max_time = acc_distance_array[total_acc_readings-1].timestamp;
	}

	fusion_data[0].timestamp = min_time;
	fusion_data[0].distance = 0;

	while (temp1 < max_time)
	{
		/*THIS CODE CAN BE MODIFIED TO LINK PROJECT 1 TO USE WIFI POSITIONING FOR INDOOR*/
		if ((fabs(temp1 - wifi_data[wifi_index].timestamp) < 200) && wifi_data[wifi_index].wifi_power > WIFI_POWER_THRESHOLD)
		{
			//printf("\nKnown Wifi AP within threshold power level is detected, can use project-1 algo to find positioning with Inertial sensors. wifi_power_level =%f",wifi_data[wifi_index].wifi_power);
			wifi_index++;
			//send_to_project_1_code (wifi_data[wifi_index].wifi_power, wifi_data[wifi_index].mac);
		}

		printf("\nCurrent time = %fms", fusion_data[total_fused_readings].timestamp);

		if (temp1 < gps_distance_array[total_gps_readings-1].timestamp && gps_idx < total_gps_readings)
		{
			//printf("\nCurrent time is less than max recorded GPS time, so we will find the next recorded GPS time value");
			gps_diff = find_gps_step_time(fusion_data[total_fused_readings].timestamp, gps_distance_array);
			//printf("\nNext GPS time is %fms away from current time", gps_diff);
			if (gps_diff != INVALID_VALUE && gps_diff < GPS_TIME_TRANSITION_MS)
			{
				//printf("\nThis time difference is valid for to consider the GPS values. We won't consider GPS values if next sample is too far");
				gps_distance = find_step_distance(fusion_data[total_fused_readings].timestamp, fusion_data[total_fused_readings].timestamp + gps_diff, gps_distance_array);
				acc_distance = find_step_distance(fusion_data[total_fused_readings].timestamp, fusion_data[total_fused_readings].timestamp + gps_diff, acc_distance_array);

				//printf("\nFrom current time to next time, estimated-->  GPS distance= %f, ACC distance = %f", gps_distance, acc_distance);

				if ((gps_distance - acc_distance) >= GPS_ACC_TRANSITION_THRESHOLD && (gps_distance - acc_distance) <= 3) //gps is higher but within 3m
				{
					printf("\033[0;31m");
					printf("\n Prioritizing GPS values here since the difference between results is within valid threshold for GPS trust\n");
					printf("\033[0m");
					fusion_data[total_fused_readings + 1].distance = gps_distance;
					fusion_data[total_fused_readings + 1].timestamp = fusion_data[total_fused_readings].timestamp + gps_diff;
				}
				else if ((gps_distance - acc_distance) >= GPS_ACC_TRANSITION_THRESHOLD && (gps_distance - acc_distance) > 3)
				{
					printf("\033[0;33m");
					printf("\n Prioritizing the ACC and Discarding the GPS results since the variation with ACC results is higher than allowed threshold\n");
					printf("\033[0m");
					fusion_data[total_fused_readings + 1].distance = acc_distance;
					fusion_data[total_fused_readings + 1].timestamp = fusion_data[total_fused_readings].timestamp + gps_diff;

				}
				else if ((gps_distance - acc_distance) < GPS_ACC_TRANSITION_THRESHOLD) //gps present but false values
				{
					printf("\033[0;33m");
					printf("\nPrioritizing the ACC values since the difference betweeen two values is less than threshold for GPS trust\n");
					printf("\033[0m");
					fusion_data[total_fused_readings + 1].distance = acc_distance;
					fusion_data[total_fused_readings + 1].timestamp = fusion_data[total_fused_readings].timestamp + gps_diff;
				}

				gps_idx++;
			}
			else
			{
				printf("\033[0;33m");
				printf("\nThis time difference is valid for to consider the GPS values. GPS next sample is too far. Prioritizing ACC values");
				printf("\033[0m");
				acc_diff = find_acc_step_time(fusion_data[total_fused_readings].timestamp, acc_distance_array);
				//printf("\nNext ACC time is %fms away from current time", acc_diff);
				acc_distance = find_step_distance(fusion_data[total_fused_readings].timestamp, fusion_data[total_fused_readings].timestamp + acc_diff, acc_distance_array);
				//printf("\nFrom current time to next time, estimated-->  ACC distance = %f\n", acc_distance);
				fusion_data[total_fused_readings + 1].distance = acc_distance;
				fusion_data[total_fused_readings + 1].timestamp = fusion_data[total_fused_readings].timestamp + acc_diff;
			}
			temp1 = fusion_data[total_fused_readings + 1].timestamp;
			total_fused_readings++;
		}
		else
		{
			printf("\033[0;33m");
			printf("\nPrioritizing the ACC values, Current time is beyond the maximum recorded time for GPS values");
			printf("\033[0m");
			acc_diff = find_acc_step_time(fusion_data[total_fused_readings].timestamp, acc_distance_array);
			//printf("\nNext ACC time is %fms away from current time", acc_diff);
			acc_distance = find_step_distance(fusion_data[total_fused_readings].timestamp, fusion_data[total_fused_readings].timestamp + acc_diff, acc_distance_array);
			//printf("\nFrom current time to next time, estimated-->  ACC distance = %f\n", acc_distance);
			fusion_data[total_fused_readings + 1].distance = acc_distance;
			fusion_data[total_fused_readings + 1].timestamp = fusion_data[total_fused_readings].timestamp + acc_diff;

			temp1 = fusion_data[total_fused_readings + 1].timestamp;
			total_fused_readings++;

		}
	}
	float distance_tot = 0, d1=0, d2=0;
	for (int i = 0; i < total_fused_readings;i++)
	{
		//printf("\nEstimated fused incremental distance at t=%fms   is =%f", fusion_data[i].timestamp, fusion_data[i].distance);
		distance_tot += fusion_data[i].distance;
	}
	for (int i = 0; i < total_acc_readings;i++)
	{
		//printf("\nEstimated ACC incremental distance at t=%fms   is =%f", acc_distance_array[i].timestamp, acc_distance_array[i].distance);
		d1 += acc_distance_array[i].distance;
	}
	for (int i = 0; i < total_gps_readings;i++)
	{
		//printf("\nEstimated GPS incremental distance at t=%fms   is =%f", gps_distance_array[i].timestamp, gps_distance_array[i].distance);
		d2 += gps_distance_array[i].distance;
	}
	printf("\033[0;32m");
	printf("\n\n\nOverall Predicted distance= %fm\n", distance_tot);
	printf("\033[0m");
	printf("\033[0;36m");
	printf("Predicted Acc distance= %fm\nPredicted GPS distance= %fm\n\n\n",d1,d2);
	printf("\033[0m");
}

void write_processed_distance_data_to_file(distance_info *fusion_data, distance_info*acceleration_data, distance_info*gps_data)
{
	FILE* fp = fopen("C:\\Users\\Areeb\\OneDrive\\Desktop\\Sensor_Fusion_Data\\distance_data.csv", "w+");
	char d1[30] = "test";
	char d2[30] = "test";
	char d3[30] = "test";
	char t1[30] = "test";
	char t2[30] = "test";
	char t3[30] = "test";
	if (fp == NULL)
	{
		//printf("Error opening the file");
		return;
	}

	for (int i = 0; i < total_acc_readings;i++)
	{
		_gcvt(acceleration_data[i].timestamp, 7, t1);
		_gcvt(acceleration_data[i].distance, 7, d1);

		fprintf(fp, "%s,%s\n", t1, d1);
	}

	for (int i = 0; i < total_gps_readings;i++)
	{
		_gcvt(gps_data[i].timestamp, 7, t2);
		_gcvt(gps_data[i].distance, 7, d2);

		fprintf(fp, ",,%s,%s\n", t2, d2);
	}

	for (int i = 0; i < total_fused_readings;i++)
	{
		_gcvt(fusion_data[i].timestamp, 7, t3);
		_gcvt(fusion_data[i].distance, 7, d3);
		fprintf(fp, ",,,,%s,%s\n", t3, d3);
	}

	//for (int i = 0;i < total_acc_readings;i++)
	//{
	//	if (i >= total_gps_readings && i < total_fused_readings)
	//	{
	//		fprintf(fp, "%s,%s,,,%s,%s\n", t1, d1, t3, d3);
	//	}
	//}
	//fprintf(fp, "%s,%s,%s,%s,%s,%s\n", t1, d1, t2, d2, t3, d3);
	// close the file
	fclose(fp);
}


int main() 
{
	acc_data acceleration_data[TOTAL_READINGS];
	gps_data gps_data[TOTAL_READINGS];
	mag_data mag_data[TOTAL_READINGS];
	gyro_data gyr_data[TOTAL_READINGS];
	wifi_data wifi_data[TOTAL_READINGS];
	//float angles [TOTAL_READINGS], gyr_angles [TOTAL_READINGS];
	direction_info direction_array[TOTAL_READINGS];
	angle_info angle_array[TOTAL_READINGS];
	distance_info acc_distance_array[TOTAL_READINGS], gps_distance_array[TOTAL_READINGS], fusion_data [TOTAL_READINGS];
	float x[TOTAL_READINGS];
	float y[TOTAL_READINGS];

	int zero_cross,no_of_steps;
	float distance_from_acc;

	read_acc_data_from_file(acceleration_data);
	read_gps_data_from_file(gps_data);
	read_mag_data_from_file(mag_data);
	read_gyro_data_from_file(gyr_data);
	read_wifi_data_from_file(wifi_data);

	scale_time_of_measurements(acceleration_data, mag_data, gps_data, gyr_data,wifi_data);

	
	find_direction_with_gyro(gyr_data, direction_array);
	find_distance_from_acc(acceleration_data, acc_distance_array);
	find_distance_with_gps(gps_data, gps_distance_array);
	find_distance_with_fusion(acc_distance_array, gps_distance_array, fusion_data,wifi_data);
	find_angles_with_mag(mag_data, angle_array, direction_array,fusion_data[0].timestamp);
	
	find_xy_coordinates(direction_array, fusion_data, angle_array, x, y);

	write_processed_acc_data_to_file(acceleration_data);
	write_processed_gps_data_to_file(gps_data);
	write_processed_mag_data_to_file(mag_data);
	write_processed_gyro_data_to_file(gyr_data);
	write_processed_wifi_data_to_file(wifi_data);
	write_processed_distance_data_to_file(fusion_data, acc_distance_array, gps_distance_array);

	return 0;
}