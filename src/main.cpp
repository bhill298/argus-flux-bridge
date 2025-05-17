#include <cstdint>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

#include <windows.h>

#include "hidapi.h"
#include "argus_monitor_data_accessor.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))

// Antec Flux Pro USB constants
const USHORT VENDOR_ID = 0x2022;
const USHORT PRODUCT_ID = 0x0522;
const BYTE PAYLOAD_HEADER[] = {0x00, 0x55, 0xAA, 0x01, 0x01, 0x06};

// other globals
bool g_output_enabled = true;
hid_device* g_antec_hid_device = nullptr;
// how long to wait when polling argus monitor (ms)
// the display will turn off if it hasn't received a new message after about 1500ms
const DWORD ARGUS_POLLING_INTERVAL = 1200;
const std::uint32_t ARGUS_SIGNATURE = 0x4D677241;

// Init global data. Return TRUE on success, FALSE on failure.
BOOL antec_hid_init() {
    if (hid_init() != 0) {
        std::cerr << "hid_init failed" << std::endl;
        return FALSE;
    }

    // open the HID device with vendor ID 8226 and product ID 1314
    g_antec_hid_device = hid_open(0x2022, 0x0522, nullptr);
    if (g_antec_hid_device == nullptr) {
        std::cerr << "Failed to find USB device" << std::endl;
        return FALSE;
    }
    return TRUE;
}

void encode_usb_temperature(double temp, std::vector<BYTE>& payload) {
    // consider a negative temperature to be an error and send "---"
    if (temp < 0.0) {
        payload.push_back(238);
        payload.push_back(238);
        payload.push_back(238);
        return;
    }
    // the display only has three digits (two digits + one decimal point), so output the max if the temp exceeds it
    else if (temp >= 100.0) {
        temp = 99.9;
    }

    // format the temperature as a string with format "XX.X" and zero pad if it's < 10.0
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(1) << std::setfill('0') << std::setw(4) << temp;
    std::string str = stream.str();

    // extract individual digits and store as integer value and push digits to payload
    payload.push_back(static_cast<BYTE>(str[0] - '0'));
    payload.push_back(static_cast<BYTE>(str[1] - '0'));
    payload.push_back(static_cast<BYTE>(str[3] - '0'));
}

std::vector<BYTE> generate_usb_payload(double cpu_temp, double gpu_temp) {
    // the format of the payload is 6 bytes of fixed header, 3 bytes cpu temp, 3 bytes gpu temp, one byte checksum
    std::vector<BYTE> payload(16);
    payload.assign(PAYLOAD_HEADER, PAYLOAD_HEADER + ARRAY_SIZE(PAYLOAD_HEADER));

    encode_usb_temperature(cpu_temp, payload);
    encode_usb_temperature(gpu_temp, payload);

    // calculate checksum (sum of all bytes mod 256)
    BYTE checksum = std::accumulate(payload.begin(), payload.end(), 0);
    payload.push_back(checksum);
    return payload;
}

void send_usb_data(const std::vector<BYTE>& usb_data)
{
    int sent = hid_write(g_antec_hid_device, usb_data.data(), usb_data.size());
    if (sent == -1) {
        std::cerr << "Failed to write to USB device" << std::endl;
    }
}

void get_temperatures(argus_monitor::data_api::ArgusMonitorData const& new_sensor_data) {
    // encode negative temperature as an error
    double cpu_temp = -1.0;
    double gpu_temp = -1.0;
    const wchar_t *gpu_name;

    // check if Argus Monitor is active
    if (new_sensor_data.Signature != ARGUS_SIGNATURE) {
        std::cerr << "Argus Monitor is not active" << std::endl;
        return;
    }

    // get CPU temperature
    if (new_sensor_data.SensorCount[argus_monitor::data_api::SENSOR_TYPE_CPU_TEMPERATURE] > 0) {
        // get the first CPU temp
        uint32_t offset = new_sensor_data.OffsetForSensorType[argus_monitor::data_api::SENSOR_TYPE_CPU_TEMPERATURE];
        cpu_temp = new_sensor_data.SensorData[offset].Value;
    }
    else {
        std::cerr << "Couldn't find CPU temperature sensors" << std::endl;
    }

    // get GPU temperature
    // index 0-1 from SENSOR_TYPE_GPU_TEMPERATURE is GPU 0 temp and hotspot,
    // 2-3 is GPU 1 temp and hotspot, etc.; each has a GPU_NAME associated with it
    // just get the first one for now
    if (new_sensor_data.SensorCount[argus_monitor::data_api::SENSOR_TYPE_GPU_TEMPERATURE] > 0) {
        // get the first GPU temp
        uint32_t offset = new_sensor_data.OffsetForSensorType[argus_monitor::data_api::SENSOR_TYPE_GPU_TEMPERATURE];
        gpu_temp = new_sensor_data.SensorData[offset].Value;
        if (g_output_enabled) {
            // Get the first GPU name
            offset = new_sensor_data.OffsetForSensorType[argus_monitor::data_api::SENSOR_TYPE_GPU_NAME];
            gpu_name = new_sensor_data.SensorData[offset].Label;
        }
    }
    else {
        gpu_name = L"";
        std::cerr << "Couldn't find GPU temperature sensors" << std::endl;
    }

    if (g_output_enabled) {
        std::wcout << "CPU: " << cpu_temp << "C, GPU (" << gpu_name << "): " << gpu_temp << "C" << std::endl;
    }
    std::vector<BYTE> payload = generate_usb_payload(cpu_temp, gpu_temp);
    send_usb_data(payload);
}

int main(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "silent") == 0) {
        g_output_enabled = false;
    }

    if (antec_hid_init() == FALSE) {
        std::cerr << "Init failed" << std::endl;
        return -1;
    }
    
    argus_monitor::data_api::ArgusMonitorDataAccessor data_accessor{};
    data_accessor.SetPollingInterval(ARGUS_POLLING_INTERVAL);
    data_accessor.RegisterSensorCallbackOnDataChanged(get_temperatures);
    while (!data_accessor.Open()) {
        if (g_output_enabled) {
            std::cout << "Waiting for Argus Monitor API connection to become available\n";
        }
        Sleep(5000);
    }

    // loop forever letting the sensor callback run
    while(TRUE) {
        Sleep(INFINITE);
    }

    // this should never run, but if it did this is what cleanup would look like
    data_accessor.Close();
    hid_close(g_antec_hid_device);
    hid_exit();

    return 0;
}
