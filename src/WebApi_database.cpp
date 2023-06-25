// SPDX-License-Identifier: GPL-2.0-or-later

#include "WebApi_database.h"
#include "Datastore.h"
#include "MessageOutput.h"
#include "WebApi.h"
#include "defaults.h"
#include <AsyncJson.h>
#include <LittleFS.h>

void WebApiDatabaseClass::init(AsyncWebServer* server)
{
    using std::placeholders::_1;

    _server = server;
    _server->on("/api/database", HTTP_GET, std::bind(&WebApiDatabaseClass::onDatabase, this, _1));
    _server->on("/api/databaseHour", HTTP_GET, std::bind(&WebApiDatabaseClass::onDatabaseHour, this, _1));
    _server->on("/api/databaseDay", HTTP_GET, std::bind(&WebApiDatabaseClass::onDatabaseDay, this, _1));
}

void WebApiDatabaseClass::loop()
{
    if (!Hoymiles.isAllRadioIdle()) {
        return;
    }
    write(Datastore.getTotalAcYieldTotalEnabled()); // write value to database
}

bool WebApiDatabaseClass::write(float energy)
{
    static uint8_t old_hour = 255;
    static float old_energy = 0.0;

    // LittleFS.remove(DATABASE_FILENAME);

    // MessageOutput.println(energy, 6);

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 5)) {
        return false;
    }
    if (timeinfo.tm_hour == old_hour) // must be new hour
        return (false);
    if (old_hour == 255) { // don't write to database after reboot
        old_hour = timeinfo.tm_hour;
        return (false);
    }
    // MessageOutput.println("Next hour.");
    if (energy <= old_energy) // enery must have increased
        return (false);
    // MessageOutput.println("Energy difference > 0");

    struct pvData d;
    d.tm_hour = old_hour;
    old_hour = timeinfo.tm_hour;
    d.tm_year = timeinfo.tm_year - 100; // year counting from 2000
    d.tm_mon = timeinfo.tm_mon + 1;
    d.tm_mday = timeinfo.tm_mday;
    d.energy = old_energy = energy;

    // create database file if it does not exist
    // if (!LittleFS.exists(DATABASE_FILENAME)) {
    //    MessageOutput.println("Database file does not exist.");
    //    File f = LittleFS.open(DATABASE_FILENAME, "w", true);
    //    f.flush();
    //    f.close();
    //    MessageOutput.println("New database file created.");
    //}

    File f = LittleFS.open(DATABASE_FILENAME, "a", true);
    if (!f) {
        MessageOutput.println("Failed to append to database.");
        return (false);
    }
    f.write((const uint8_t*)&d, sizeof(pvData));
    f.close();
    // MessageOutput.println("Write data point.");
    return (true);
}

// read chunk from database
size_t WebApiDatabaseClass::readchunk(uint8_t* buffer, size_t maxLen, size_t index)
{
    static bool first = true;
    static bool last = false;
    static File f;
    uint8_t* pr = buffer;
    uint8_t* pre = pr + maxLen - 50;
    size_t r;
    struct pvData d;

    if (first) {
        f = LittleFS.open(DATABASE_FILENAME, "r", false);
        if (!f) {
            return (0);
        }
        *pr++ = '[';
    }
    while (true) {
        r = f.read((uint8_t*)&d, sizeof(pvData)); // read from database
        if (r <= 0) {
            if (last) {
                f.close();
                first = true;
                last = false;
                return (0); // end transmission
            }
            last = true;
            *pr++ = ']';
            return (pr - buffer); // last chunk
        }
        if (first) {
            first = false;
        } else {
            *pr++ = ',';
        }
        int len = sprintf((char*)pr, "[%d,%d,%d,%d,%f]",
            d.tm_year, d.tm_mon, d.tm_mday, d.tm_hour, d.energy * 1e3);
        if (len >= 0) {
            pr += len;
        }
        if (pr >= pre)
            return (pr - buffer); // buffer full, return number of chars
    }
}

size_t WebApiDatabaseClass::readchunk_log(uint8_t* buffer, size_t maxLen, size_t index)
{
    size_t x = readchunk(buffer, maxLen, index);
    MessageOutput.println("----------");
    MessageOutput.println(maxLen);
    MessageOutput.println(x);
    return (x);
}

// read chunk from database for the last 25 hours
size_t WebApiDatabaseClass::readchunkHour(uint8_t* buffer, size_t maxLen, size_t index)
{
    time_t now;
    time(&now);
    time_t sd = now - (60 * 60 * 25); // subtract 25h
    struct tm startday;
    localtime_r(&sd, &startday);
    if (startday.tm_year <= (2016 - 1900)) {
        return (false); // time not set
    }

    static bool first = true;
    static bool last = false;
    static bool valid = false;
    static float oldenergy = 0.0;
    static File f;
    uint8_t* pr = buffer;
    uint8_t* pre = pr + maxLen - 50;
    size_t r;
    struct pvData d;

    if (first) {
        f = LittleFS.open(DATABASE_FILENAME, "r", false);
        if (!f) {
            return (0);
        }
        *pr++ = '[';
    }
    while (true) {
        r = f.read((uint8_t*)&d, sizeof(pvData)); // read from database
        if (r <= 0) {
            if (last) {
                f.close();
                first = true;
                last = false;
                valid = false;
                return (0); // end transmission
            }
            last = true;
            *pr++ = ']';
            return (pr - buffer); // last chunk
        }
        if (valid) {
            if (first) {
                first = false;
                oldenergy = d.energy;
            } else {
                *pr++ = ',';
            }
            int len = sprintf((char*)pr, "[%d,%d,%d,%d,%f]",
                d.tm_year, d.tm_mon, d.tm_mday, d.tm_hour, 
                (d.energy - oldenergy) * 1e3);
            oldenergy = d.energy;
            if (len >= 0) {
                pr += len;
            }
            if (pr >= pre)
                return (pr - buffer); // buffer full, return number of chars
        } else {
            if ((d.tm_year >= startday.tm_year - 100)
                && (d.tm_mon >= startday.tm_mon + 1)
                && (d.tm_mday >= startday.tm_mday)
                && (d.tm_hour >= startday.tm_hour)
            )
                valid = true;
        }
    }
}

// read chunk from database for calendar view
size_t WebApiDatabaseClass::readchunkDay(uint8_t* buffer, size_t maxLen, size_t index)
{
    static bool first = true;
    static bool last = false;
    static float startenergy = 0.0;
    static struct pvData endofday = { 0, 0, 0, 0, 0.0 };
    static File f;
    uint8_t* pr = buffer;
    uint8_t* pre = pr + maxLen - 50;
    size_t r;
    struct pvData d;

    if (first) {
        f = LittleFS.open(DATABASE_FILENAME, "r", false);
        if (!f) {
            return (0);
        }
        *pr++ = '[';
    }
    while (true) {
        r = f.read((uint8_t*)&d, sizeof(pvData)); // read from database
        if (r <= 0) {
            if (last) {
                f.close();
                first = true;
                last = false;
                endofday = { 0, 0, 0, 0, 0.0 };
                startenergy = 0.0;
                return (0); // end transmission
            }
            last = true;
            if (!first)
                *pr++ = ',';
            int len = sprintf((char*)pr, "[%d,%d,%d,%d,%f]",
                endofday.tm_year, endofday.tm_mon, endofday.tm_mday, endofday.tm_hour, 
                (endofday.energy - startenergy) * 1e3);
            pr += len;
            *pr++ = ']';
            return (pr - buffer); // last chunk
        }
        if (endofday.tm_year == 0) {
            startenergy = d.energy;
        } else {
            if (endofday.tm_mday != d.tm_mday) { // next day
                if (first) {
                    first = false;
                } else
                    *pr++ = ',';
                int len = sprintf((char*)pr, "[%d,%d,%d,%d,%f]",
                    endofday.tm_year, endofday.tm_mon, endofday.tm_mday, endofday.tm_hour, 
                    (endofday.energy - startenergy) * 1e3);
                startenergy = endofday.energy;
                if (len >= 0)
                    pr += len;
                if (pr >= pre)
                    return (pr - buffer); // buffer full, return number of chars
            }
        }
        endofday = d;
    }
}

void WebApiDatabaseClass::onDatabase(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }
    AsyncWebServerResponse* response = request->beginChunkedResponse("application/json", readchunk);
    request->send(response);
}

void WebApiDatabaseClass::onDatabaseHour(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }
    AsyncWebServerResponse* response = request->beginChunkedResponse("application/json", readchunkHour);
    request->send(response);
}

void WebApiDatabaseClass::onDatabaseDay(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }
    AsyncWebServerResponse* response = request->beginChunkedResponse("application/json", readchunkDay);
    request->send(response);
}

/*  JS
const energy = JSON.parse('[[23,5,19,6,175.4089966],[23,5,19,10,175.7649994],[23,5,19,10,175.8939972],[23,5,19,10,175.904007],[23,5,19,10,175.9149933],[23,5,19,11,175.9669952],[23,5,19,11,175.973999],[23,5,19,12,176.1159973],[23,5,19,13,176.2900085],[23,5,19,14,176.4179993],[23,5,19,15,176.5429993],[23,5,19,16,176.6100006],[23,5,19,17,176.7269897],[23,5,19,18,176.8370056],[23,5,19,19,176.9060059],[23,5,19,20,176.9360046],[23,5,19,21,176.9459991],[23,5,20,5,176.9459991],[23,5,20,6,176.9470062],[23,5,20,7,176.9629974],[23,5,20,8,177.0270081],[23,5,20,9,177.0899963],[23,5,20,10,177.2389984],[23,5,20,11,177.4759979],[23,5,20,12,177.7310028],[23,5,20,13,178.0749817],[23,5,20,14,178.3970032],[23,5,20,15,178.7460022],[23,5,20,16,179.0829926],[23,5,20,17,179.3110046],[23,5,20,18,179.4849854],[23,5,20,19,179.5549927],[23,5,20,20,179.5899963]]')

var end = new Date()
var start = new Date()
var interval = 1
start.setDate(end.getDate() - interval)
start.setHours(start.getHours() - 1)
console.log(start.toLocaleString())
console.log(end.toLocaleString())

google.charts.load('current', {
  packages: ['corechart', 'bar']
});
google.charts.setOnLoadCallback(drawBasic);

function drawBasic() {
  var data = new google.visualization.DataTable();
  data.addColumn('datetime', 'Time');
  data.addColumn('number', 'Energy');
  var old_energy = energy[0][4]
  energy.forEach((x) => {
    var d = new Date(x[0] + 2000, x[1] - 1, x[2], x[3])
    if ((d >= start) && (d <= end)) {
      data.addRow([d, (x[4] - old_energy) * 1000])
    }
    old_energy = x[4]
  })

  var date_formatter = new google.visualization.DateFormat({
    pattern: "dd.MM.YY HH:mm"
  });
  date_formatter.format(data, 0);

  var options = {
    height: 500,
    legend: {
      position: 'none'
    },
    hAxis: {
      format: 'dd.MM.YY HH:mm',
      slantedText: true
    },
    vAxis: {
      format: '# Wh'
    }
  }

  var chart = new google.visualization.ColumnChart(document.getElementById('chart_div'));
  chart.draw(data, options);
}
*/

/* HTML
  <script type="text/javascript" src="https://www.gstatic.com/charts/loader.js"></script>
  <div id="chart_div"></div>
*/
