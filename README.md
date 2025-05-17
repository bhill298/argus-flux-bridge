# Argus Flux Bridge

This project reads CPU and GPU temperatures from Argus Monitor and sends this data to an [Antec Flux Pro](https://www.antec.com/product/case/flux-pro) temperature display. It makes use of [HIDAPI](https://github.com/libusb/hidapi/tree/master) and the [Argus Monitor Data API](https://github.com/argotronic/argus_data_api). I also wrote this using information from [this blog post](https://nishtahir.com/building-an-ubuntu-service-for-my-antec-flux-display/).
To use this, you will need the to enable the API in Argus Monitor with `Settings -> Stability -> Enable Argus Monitor Data API`.

If you want to have this run on startup and run in the background, the easiest way is to use Task Scheduler:
1. `Create Basic Task...`
1. Trigger: `When I log on`
1. Action: `Start a program` set to `\path\to\argus_flux_bridge.exe` with argument `silent`
1. Check `Open the Properties dialog for this task when I click Finish` and click Finish
1. Under General, select `Run whether the user is logged on or not` and tick `Do not store password...`. This will make it run silently in the background.

You could also look at using something like [minimize-to-tray](https://github.com/danielgjackson/minimize-to-tray) if you want it to run in the system tray.

## Building

1.  Clone the repository.
1.  Open the project in Visual Studio Code (launched from a Visual Studio Developer Command Prompt).
    - You will need either Visual Studio or Build Tools for Visual Studio with C++ development tools installed.
1.  Build the project
    -   Use `Ctrl+Shift+B` (or the `Tasks: Run Build Task` command) and select the build task.
    -   Alternatively, refer to the `cl.exe` build command specified in `tasks.json` to build manually.