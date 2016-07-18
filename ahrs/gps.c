/*
diy-efis
Copyright (C) 2016 Kotuku Aerospace Limited

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

If a file does not contain a copyright header, either because it is incomplete
or a binary file then the above copyright notice will apply.

Portions of this repository may have further copyright notices that may be
identified in the respective files.  In those cases the above copyright notice and
the GPL3 are subservient to that copyright notice.

Portions of this repository contain code fragments from the following
providers.


If any file has a copyright notice or portions of code have been used
and the original copyright notice is not yet transcribed to the repository
then the origional copyright notice is to be respected.

If any material is included in the repository that is not open source
it must be removed as soon as possible after the code fragment is identified.
*/
#include "ahrs.h"
#include "gps.h"

#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "../dspic33f-lib/analog_devices.h"
#include "../dspic33f-lib/uart_device.h"
#include "../can-aerospace/can_aerospace.h"
#include "../dspic-lib/publisher.h"

// this holds each line as it is read.
// the termination is a \n

// these hold the computed variables.  The first entry is the airspeed
// located in ahrs.c
extern analog_channel_definition_t channel_definitions[];
event_t gps_position_updated = {
  .flags = EVENT_AUTORESET 
  };

static uint8_t process_line(struct _uart_config_t *, char *buffer, uint8_t sentence, uint8_t word);

#define NUM_RX_BUFFERS  8
#define RX_BUFFER_LENGTH 128

#define RX_QUEUE_LENGTH (NUM_RX_BUFFERS * RX_BUFFER_LENGTH)

static uint8_t rx_buffer[RX_BUFFER_LENGTH];
static uint8_t rx_worker_buffer[RX_BUFFER_LENGTH];
static uint8_t rx_queue[RX_QUEUE_LENGTH];

static uart_config_t uart_config = {
  .uart_number = 1,
  .flags = NMEA_DECODER_ENABLE,
  .rate = 9600,
  .eol_char = '\n',
  .rx_buffer = rx_buffer,
  .rx_worker_buffer = rx_worker_buffer,
  .rx_length = RX_BUFFER_LENGTH,
  .callback.nmea_callback = process_line,
  .rx_pin = rpi_rpi25,
  .tx_pin = RP42R,
  };

void gps_init(uint16_t *rx_stack,
                     uint16_t rx_stack_size,
                     int8_t *rx_pid,
                     uint16_t *tx_stack,
                     uint16_t tx_stack_size,
                     int8_t *tx_pid)
  {
  ahrs_state.gps_position_valid = false;
  ahrs_state.gps_ground_speed = 0;
  ahrs_state.gps_ground_speed_valid = false;
  ahrs_state.time = 0;
  ahrs_state.gps_track = 0;
  ahrs_state.gps_track_valid = false;
  ahrs_state.wind_direction = 0;
  ahrs_state.wind_speed = 0;
  
  init_deque(&uart_config.rx_queue, RX_BUFFER_LENGTH, rx_queue, NUM_RX_BUFFERS);
  
  // create the left mag worker
  init_uart(&uart_config, rx_stack, rx_stack_size, tx_stack, tx_stack_size);
  if(rx_pid != 0)
    *rx_pid = uart_config.rx_worker_pid;
  
  if(tx_pid != 0)
    *tx_pid = uart_config.tx_worker_pid;
  }

bool gps_config(semaphore_t *worker_task)
  {
  signal(worker_task);
  return true;
  }
  
// returns the result in radians
// increments pointer to \0 at end of value
static float parse_nmea_degrees(char **buffer)
  {
  float result = 0.0;
  char *start = *buffer;
  char *degrees = *buffer;
  int deg_value;

  // find the . character and convert to a \n
  while (*start != 0)
    if (*start == '.')
      break;
    else
      start++;

  // error as not a valid string
  if (*start != '.')
    return result;

  start -= 2; // minutes are always 2 chars
  result = strtof(start, buffer); // format is mm.mmmm

  result /= 60.0; // make 1.0 == 1 degree

  // terminate the degrees part
  *start = 0;

  deg_value = atoi(degrees);

  result += deg_value;

  return degrees_to_radians(result);
  }

static uint8_t process_line(struct _uart_config_t *config, char *buffer, uint8_t sentence, uint8_t word)
  {
  if (sentence == 0)
    {
    if (strcmp(buffer, "GPGGA") == 0)
      {
      /*
      GGA - essential fix data which provide 3D location and accuracy data.

      $GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47

      Where:
           GGA          Global Positioning System Fix Data
           123519       Fix taken at 12:35:19 UTC
           4807.038,N   Latitude 48 deg 07.038' N
           01131.000,E  Longitude 11 deg 31.000' E
           1            Fix quality: 0 = invalid
                                     1 = GPS fix (SPS)
                                     2 = DGPS fix
                                     3 = PPS fix
                   4 = Real Time Kinematic
                   5 = Float RTK
                                     6 = estimated (dead reckoning) (2.3 feature)
                   7 = Manual input mode
                   8 = Simulation mode
           08           Number of satellites being tracked
           0.9          Horizontal dilution of position
           545.4,M      Altitude, Meters, above mean sea level
           46.9,M       Height of geoid (mean sea level) above WGS84
                            ellipsoid
           (empty field) time in seconds since last DGPS update
           (empty field) DGPS station ID number
       *47          the checksum data, always begins with *

      If the height of geoid is missing then the altitude should be suspect. Some non-standard
      implementations report altitude with respect to the ellipsoid rather than geoid altitude.
      Some units do not report negative altitudes at all. This is the only sentence that reports
      altitude.
       */
      return 1;
      }
    else if (strcmp(buffer, "GPGSV") == 0)
      {
      /*
      GSV - Satellites in View shows data about the satellites that the unit might
      be able to find based on its viewing mask and almanac data. It also shows current
      ability to track this data. Note that one GSV sentence only can provide data for
      up to 4 satellites and thus there may need to be 3 sentences for the full
      information. It is reasonable for the GSV sentence to contain more satellites
      than GGA might indicate since GSV may include satellites that are not used as
      part of the solution. It is not a requirment that the GSV sentences all appear in
      sequence. To avoid overloading the data bandwidth some receivers may place the
      various sentences in totally different samples since each sentence identifies
      which one it is.

      The field called SNR (Signal to Noise Ratio) in the NMEA standard is often
      referred to as signal strength. SNR is an indirect but more useful value that
      raw signal strength. It can range from 0 to 99 and has units of dB according
      to the NMEA standard, but the various manufacturers send different ranges of
      numbers with different starting numbers so the values themselves cannot necessarily
      be used to evaluate different units. The range of working values in a given gps
      will usually show a difference of about 25 to 35 between the lowest and highest
      values, however 0 is a special case and may be shown on satellites that are in
      view but not being tracked.

        $GPGSV,2,1,08,01,40,083,46,02,17,308,41,12,07,344,39,14,22,228,45*75

      Where:
            GSV          Satellites in view
            2            Number of sentences for full data
            1            sentence 1 of 2
            08           Number of satellites in view

            01           Satellite PRN number
            40           Elevation, degrees
            083          Azimuth, degrees
            46           SNR - higher is better
                 for up to 4 satellites per sentence
       *75          the checksum data, always begins with *

       */
      return 2;
      }
    else if (strcmp(buffer, "GPGSA") == 0)
      {
      /*
      GSA - GPS DOP and active satellites. This sentence provides details on the nature
      of the fix. It includes the numbers of the satellites being used in the current
      solution and the DOP. DOP (dilution of precision) is an indication of the effect
      of satellite geometry on the accuracy of the fix. It is a unitless number where
      smaller is better. For 3D fixes using 4 satellites a 1.0 would be considered to
      be a perfect number, however for overdetermined solutions it is possible to see
      numbers below 1.0.

      There are differences in the way the PRN's are presented which can effect the
      ability of some programs to display this data. For example, in the example shown
      below there are 5 satellites in the solution and the null fields are scattered
      indicating that the almanac would show satellites in the null positions that are
      not being used as part of this solution. Other receivers might output all of the
      satellites used at the beginning of the sentence with the null field all stacked
      up at the end. This difference accounts for some satellite display programs not
      always being able to display the satellites being tracked. Some units may show all
      satellites that have ephemeris data without regard to their use as part of the
      solution but this is non-standard.

        $GPGSA,A,3,04,05,,09,12,,,24,,,,,2.5,1.3,2.1*39

      Where:
           GSA      Satellite status
           A        Auto selection of 2D or 3D fix (M = manual)
           3        3D fix - values include: 1 = no fix
                                             2 = 2D fix
                                             3 = 3D fix
           04,05... PRNs of satellites used for fix (space for 12)
           2.5      PDOP (dilution of precision)
           1.3      Horizontal dilution of precision (HDOP)
           2.1      Vertical dilution of precision (VDOP)
       *39      the checksum data, always begins with *

       */
      return 3;
      }
    else if (strcmp(buffer, "GPRMC") == 0)
      {
      /*
      RMC - NMEA has its own version of essential gps pvt (position, velocity, time) data.
      It is called RMC, The Recommended Minimum, which will look similar to:

      $GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A

      Where:
           RMC          Recommended Minimum sentence C
           123519       Fix taken at 12:35:19 UTC
           A            Status A=active or V=Void.
           4807.038,N   Latitude 48 deg 07.038' N
           01131.000,E  Longitude 11 deg 31.000' E
           022.4        Speed over the ground in knots
           084.4        Track angle in degrees True
           230394       Date - 23rd of March 1994
           003.1,W      Magnetic Variation
       *6A          The checksum data, always begins with *

      Note that, as of the 2.3 release of NMEA, there is a new field in the RMC sentence at the
      end just prior to the checksum. For more information on this field see here.

       */
      return 4;
      }
    }

  if (sentence == 4)
    {
    can_msg_t msg;

    switch (word)
      {
      case 1:
        // UTC Time
        if (*buffer != 0)
          {
          // Fix Taken Time
          // read 3 2 char numbers
          char num[3];
          num[0] = buffer[0];
          num[1] = buffer[1];
          num[2] = 0;

          msg.msg.canas.data[0] = (unsigned char) atoi(num);
          ahrs_state.time = 3600 * msg.msg.canas.data[0];

          buffer += 2;
          num[0] = buffer[0];
          num[1] = buffer[1];
          num[2] = 0;

          msg.msg.canas.data[1] = (unsigned char) atoi(num);
          ahrs_state.time += 60 * msg.msg.canas.data[1];

          buffer += 2;
          num[0] = buffer[0];
          num[1] = buffer[1];
          num[2] = 0;

          msg.msg.canas.data[2] = (unsigned char) atoi(num);
          ahrs_state.time += msg.msg.canas.data[2];
          buffer += 3;
          msg.id = id_def_utc;
          msg.msg.canas.data_type = CANAS_DATATYPE_UCHAR3;
          msg.flags = 7;
          publish(&msg);
          }
        break;
      case 2:
        // Status of the GPS position.
        // if we loose it the kalman filter will stop
        // and an AHRS warning given
        if(*buffer == 'A')
          ahrs_state.gps_position_valid = true;
        else
          ahrs_state.gps_position_valid = false;
        break;
      case 3:
        // Lattitude
        {
        // GPS Latitude
        ahrs_state.gps_position.lat = parse_nmea_degrees(&buffer);

        /*
        float sinsq;
        // update the gravity vector
        // WGS84 gravity model.  The effect of gravity over latitude is strong
        // enough to change the estimated accelerometer bias
        sinsq = sin(ahrs_state.pos.lat);
        sinsq *= sinsq;

        ahrs_state.gravity =
          9.7803267714f *
          (1.0f + 0.00193185138639f * sinsq) /
          sqrt(1.0f - 0.00669437999013f * sinsq)
          - 3.086e-6f * ahrs_state.pos.alt;

        set_mask(SENSORUPDATES_ga);
         * */
        }
        break;
      case 4:
        // North/South indicator
        if (*buffer == 'S')
          ahrs_state.gps_position.lat *= -1;

        // TODO: this should be sent by the ekf?
        publish_float(ahrs_state.gps_position.lat, id_gps_aircraft_lattitude);
        break;
      case 5:
        // Longitude
        ahrs_state.gps_position.lng = parse_nmea_degrees(&buffer);
        break;
      case 6:
        // East/West indicator
        if (*buffer == 'W')
          ahrs_state.gps_position.lng *= -1;

        // TODO: this should be send by the ekf?
        publish_float(ahrs_state.gps_position.lng, id_gps_aircraft_longitude);
        break;
      case 7:
        // Speed over ground
        if (*buffer != 0)
          {
          // speed in knots
          ahrs_state.gps_ground_speed = knots_to_meters(strtof(buffer, &buffer));
          publish_float(ahrs_state.gps_ground_speed, id_gps_groundspeed);
          ahrs_state.gps_ground_speed_valid = ahrs_state.gps_ground_speed > 0;
          }
        else
          ahrs_state.gps_ground_speed = 0;
        break;
      case 8:
        // track in degrees
        if (*buffer != 0)
          {
          ahrs_state.gps_track = degrees_to_radians(strtof(buffer, &buffer));
          publish_float(ahrs_state.gps_track, id_track);
          ahrs_state.gps_track_valid = true;
          }
        else
          ahrs_state.gps_track_valid = false;
        break;
      case 9:
        // Date
        if (*buffer != 0)
          {
          //date_t date;
          char num[3];
          // Date fix taken
          num[0] = buffer[0];
          num[1] = buffer[1];
          num[2] = 0;

          msg.msg.canas.data[0] = (unsigned char) atoi(num);
          buffer += 2;

          num[0] = buffer[0];
          num[1] = buffer[1];
          num[2] = 0;

          msg.msg.canas.data[1] = (unsigned char) atoi(num);
          buffer += 2;

          num[0] = buffer[0];
          num[1] = buffer[1];
          num[2] = 0;

          msg.msg.canas.data[2] = 20;
          msg.msg.canas.data[3] = (unsigned char) atoi(num);
          buffer += 2;

          msg.msg.canas.data_type = CANAS_DATATYPE_UCHAR4;
          msg.id = id_def_date;
          msg.flags = 8;
          publish(&msg);

          ahrs_state.date.day = msg.msg.canas.data[0];
          ahrs_state.date.month = msg.msg.canas.data[1];
          ahrs_state.date.year = msg.msg.canas.data[2] + 2000;
          }
        break;
      case 10:
        // magnetic variation
        break;
      case 11:
        // Variation Sense
        break;
      case 12:
        // Mode, A=autonomous, D=DGPS, N=Data not valid
        if(*buffer == 'A')
          ahrs_state.gps_position_valid = true;
        else
          ahrs_state.gps_position_valid = false;
        break;
      case 13 :
        // checksum
        if (buffer != 0)
          {
          if (ahrs_state.gps_track_valid && ahrs_state.gps_ground_speed_valid)
            {
            // we now have our magnetic_heading and our track, using triganometry
            //
            // Given:
            //   a = Wind velocity (m/s * 10)
            //   b = Airspeed (m/s * 10)
            //   c = Groundspeed (m/s * 10)
            //   H = degrees_to_radians(magnetic_heading)
            //   T = degrees_to_radians(track)
            //   A = H - T (radians)
            //   B = Angle wind to track
            //   C = Angle wind to magnetic_heading

            //
            //  a = sqrt(((b * b) + (c * c)) - (2 * b * c * cos(A)))
            //
            //  if(b < c) C = degrees_to_radians(180) - A - asin(b * (sin(A)/a))
            //  else C = asin(c * (sin(A)/a))
            //
            // therefore
            //    Wind angle (true) = radians_to_degrees(degreesto_radians(180) - ((degrees_to_radians(360) - H) + C))
            //    Wind velocity = a
            float heading_radians = degrees_to_radians(get_datapoint_float(id_magnetic_heading));
            float heading_ms = get_datapoint_float(id_indicated_airspeed);
            float track_radians = get_datapoint_float(id_track);
            float track_ms = get_datapoint_float(id_gps_groundspeed);
            float drift_angle = track_radians - heading_radians;

            while (drift_angle < 0)
              drift_angle += degrees_to_radians(360);

            while (drift_angle > degrees_to_radians(360))
              drift_angle -= degrees_to_radians(360);

            ahrs_state.wind_speed = sqrt(((heading_ms * heading_ms) + (track_ms * track_ms)) - (2 * heading_ms * track_ms * cos(drift_angle)));

            float windRadians;
            if (heading_ms < track_ms)
              windRadians = degrees_to_radians(180) - drift_angle - asin(heading_ms * (sin(drift_angle) / ahrs_state.wind_speed));
            else
              windRadians = asin(track_ms * (sin(drift_angle) / ahrs_state.wind_speed));

            ahrs_state.wind_direction = heading_radians - windRadians;

            while (ahrs_state.wind_direction < 0)
              ahrs_state.wind_direction += degrees_to_radians(360);

            while (ahrs_state.wind_direction > degrees_to_radians(360))
              ahrs_state.wind_direction -= degrees_to_radians(360);

            publish_float(ahrs_state.wind_direction, id_wind_speed);
            publish_float(ahrs_state.wind_speed, id_wind_direction);
            }

          // tell the devices that the navigation unit is running ok
          publish_short(1, id_nav_valid);

          // if the aircraft has moved more than 1 degree then re-compute the magnetic variation.
          set_event(&gps_position_updated);
          }
        else
          {
          ahrs_state.gps_position_valid = false;
          publish_short(0, id_nav_valid);
          }
        break;
      }
    }

  return sentence;
  }
