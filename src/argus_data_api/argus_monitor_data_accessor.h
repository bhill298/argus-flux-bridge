#ifndef ARGUS_MONITOR_DATA_API_ACCESSOR_H
#define ARGUS_MONITOR_DATA_API_ACCESSOR_H

#include "argus_monitor_data_api.h"
#include <mutex>
#include <thread>
#include <wchar.h>
#include <windows.h>
#include <functional>
#include <iostream>

namespace argus_monitor {
namespace data_api {

    class ArgusMonitorDataAccessor {

    private:
        HANDLE                                                               handle_file_mapping{ nullptr };
        void*                                                                pointer_to_mapped_data{ nullptr };
        argus_monitor::data_api::ArgusMonitorData const*                      sensor_data_{ nullptr };
        bool                                                                 is_open_{ false };
        bool                                                                 keep_polling_{ true };
        std::function<void(argus_monitor::data_api::ArgusMonitorData const&)> new_sensor_data_callback_{};
        std::thread                                                          polling_thread{};
        DWORD polling_interval{ 100U };


        void          StartPollingThread();
        static void   Poll(ArgusMonitorDataAccessor* class_instance);
        static HANDLE OpenArgusApiMutex();
        void          ProcessSensorData(argus_monitor::data_api::ArgusMonitorData const& sensor_data);

    public:
        ArgusMonitorDataAccessor() = default;

        ArgusMonitorDataAccessor(ArgusMonitorDataAccessor const&) = delete;
        ArgusMonitorDataAccessor(ArgusMonitorDataAccessor&&)      = delete;
        ArgusMonitorDataAccessor& operator=(ArgusMonitorDataAccessor const&) = delete;
        ArgusMonitorDataAccessor& operator=(ArgusMonitorDataAccessor&&) = delete;

        ~ArgusMonitorDataAccessor()
        {
            keep_polling_ = false;
            if (polling_thread.joinable()) {
                polling_thread.join();
            }
        }

        bool Open();
        bool IsOpen() const noexcept { return is_open_; }
        void SetPollingInterval(DWORD _polling_interval) { polling_interval = _polling_interval; }
        void Close();

        void RegisterSensorCallbackOnDataChanged(std::function<void(argus_monitor::data_api::ArgusMonitorData const&)> callback);
    };


}
}

#endif    // ARGUS_MONITOR_DATA_API_ACCESSOR_H
