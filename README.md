**Arduino Mini OS with WiFi REST API**

This project demonstrates a simple OS for Arduino that has a task scheduler using the Arduino Uno R4 WiFi (or similar boards) that can run multiple tasks on a periodic basis. It includes a REST API to start, stop, toggle, and view tasks over WiFi. A blinking LED and serial console printing are implemented as example tasks.

**Features**

Multi-task scheduler with:

Task start, stop, and toggle functionality.

Tracking of last and next execution times.

**Example tasks**:

Blinker: Toggles an LED periodically.

SerialPrinter: Prints task info to the serial monitor periodically.

WiFi Access Point mode with REST API:

View all tasks (/tasks).

View task status (/tasks/status).

Start, stop, or toggle tasks by ID (/tasks/start/<id>, /tasks/stop/<id>, /tasks/toggle/<id>).

Get detailed task info (/tasks/info/<id>).

JSON-based API responses for easy integration.

**Hardware Requirements**

Arduino Uno R4 WiFi (or compatible board with WiFiS3 library support)

LED on pin 13 (built-in LED on Uno)

USB cable for programming

**Software Requirements**

Arduino IDE (or compatible platform)

WiFiS3 library

Wiring

LED: Connect to pin 13 (onboard LED works, no external connection needed).

**Installation**

Clone this repository:

Set your WiFi SSID and password in the code:

char ssid[] = "YOUR_SSID";
char pass[] = "YOUR_PASSWORD";


Upload the code to your Arduino board.

**Usage**

Open the Serial Monitor at 115200 baud to view logs.

Connect your computer or phone to the Arduino WiFi access point.

Open a browser and access the following endpoints:

Endpoint	Method	Description
/tasks	GET	List all tasks (ID, PID, name)
/tasks/status	GET	Get current task status (running/stopped, last and next run time)
/tasks/start/<id>	GET	Start task with specified ID
/tasks/stop/<id>	GET	Stop task with specified ID
/tasks/toggle/<id>	GET	Toggle task state
/tasks/info/<id>	GET	Detailed info for a specific task

**Code Structure**

Task system: struct TCB for task control, with TaskFn function pointers.

Scheduler: scheduler_tick() executes tasks based on their period and state.

REST API: handleClient() parses HTTP requests and returns JSON responses.

**Tasks**:

task_blink() → toggles LED

task_printer() → prints task info to Serial
