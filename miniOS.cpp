#include "WiFiS3.h"

char ssid[] = "SSID";   
char pass[] = "PWD";
WiFiServer server(80);

const int LED_PIN = 13;
const unsigned long DEFAULT_BLINK_MS = 500UL;
const unsigned long DEFAULT_PRINT_MS = 1000UL;
int status = WL_IDLE_STATUS;

// ---------- Task system ----------
typedef void (*TaskFn)(int idx);
enum TaskState { TASK_STOPPED = 0, TASK_RUNNING = 1 };

struct TCB {
  int id;
  unsigned long pid;
  const char* name;
  TaskFn fn;
  TaskState state;
  unsigned long period_ms;
  unsigned long next_run_ms;
  unsigned long last_run_ms;
};

const int NUM_TASKS = 2;
TCB tasks[NUM_TASKS];
unsigned long next_pid = 1000;

// ---------- Task implementations ----------
void task_blink(int idx) {
  static bool led_on[NUM_TASKS] = { false };
  led_on[idx] = !led_on[idx];
  digitalWrite(LED_PIN, led_on[idx] ? HIGH : LOW);
}

void task_printer(int idx) {
  Serial.print("[PID ");
  Serial.print(tasks[idx].pid);
  Serial.print("] Hello from task '");
  Serial.print(tasks[idx].name);
  Serial.print("' (id=");
  Serial.print(tasks[idx].id);
  Serial.print(") at ");
  Serial.print(millis());
  Serial.println("ms");
}

// ---------- Task helpers ----------
void register_task(int idx, const char* name, TaskFn fn, unsigned long period_ms, bool start_immediately) {
  tasks[idx].id = idx;
  tasks[idx].pid = next_pid++;
  tasks[idx].name = name;
  tasks[idx].fn = fn;
  tasks[idx].state = start_immediately ? TASK_RUNNING : TASK_STOPPED;
  tasks[idx].period_ms = period_ms;
  unsigned long now = millis();
  tasks[idx].last_run_ms = 0;
  tasks[idx].next_run_ms = start_immediately ? (now + period_ms) : 0;
}

void start_task(int idx) {
  if(idx<0||idx>=NUM_TASKS) return;
  if(tasks[idx].state==TASK_RUNNING) return;
  tasks[idx].state=TASK_RUNNING;
  tasks[idx].next_run_ms=millis()+tasks[idx].period_ms;
}

void stop_task(int idx) {
  if(idx<0||idx>=NUM_TASKS) return;
  if(tasks[idx].state==TASK_STOPPED) return;
  tasks[idx].state=TASK_STOPPED;
}

void toggle_task(int idx) {
  if(idx<0||idx>=NUM_TASKS) return;
  if(tasks[idx].state==TASK_RUNNING) stop_task(idx);
  else start_task(idx);
}

// ---------- Scheduler ----------
void scheduler_tick() {
  unsigned long now=millis();
  for(int i=0;i<NUM_TASKS;i++){
    TCB &t = tasks[i];
    if(t.state!=TASK_RUNNING) continue;
    if((long)(now - t.next_run_ms)>=0){
      t.fn(i);
      t.last_run_ms = now;
      t.next_run_ms = now + t.period_ms;
    }
  }
}

void printWiFiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print where to go in a browser:
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);

}

// ---------- REST API ----------
void handleClient(WiFiClient client){
  if(!client) return;
  while(!client.available()){}
  String req = client.readStringUntil('\r');
  client.flush();

  String response;
  // /tasks
  if(req.indexOf("GET /tasks ")>=0){
    response="{\"tasks\":[";
    for(int i=0;i<NUM_TASKS;i++){
      response+="{\"id\":"+String(tasks[i].id)+",\"pid\":"+String(tasks[i].pid)+",\"name\":\""+String(tasks[i].name)+"\"}";
      if(i<NUM_TASKS-1) response+=",";
    }
    response+="]}";
  }
  // /tasks/status
  else if(req.indexOf("GET /tasks/status")>=0){
    response="{\"tasks\":[";
    for(int i=0;i<NUM_TASKS;i++){
      TCB &t=tasks[i];
      response+="{\"id\":"+String(t.id)+",\"pid\":"+String(t.pid)+",\"name\":\""+String(t.name)+"\",\"state\":\""+String(t.state==TASK_RUNNING?"RUNNING":"STOPPED")+"\",\"last_run_ms\":"+String(t.last_run_ms)+",\"next_run_ms\":"+String(t.next_run_ms)+"}";
      if(i<NUM_TASKS-1) response+=",";
    }
    response+="]}";
  }
  // /tasks/start/<id>
  else if(req.indexOf("GET /tasks/start/")>=0){
    int idx=req.substring(17).toInt();
    start_task(idx);
    response="{\"message\":\"Started task "+String(idx)+"\"}";
  }
  // /tasks/stop/<id>
  else if(req.indexOf("GET /tasks/stop/")>=0){
    int idx=req.substring(16).toInt();
    stop_task(idx);
    response="{\"message\":\"Stopped task "+String(idx)+"\"}";
  }
  // /tasks/toggle/<id>
  else if(req.indexOf("GET /tasks/toggle/")>=0){
    int idx=req.substring(18).toInt();
    toggle_task(idx);
    response="{\"message\":\"Toggled task "+String(idx)+"\"}";
  }
  // /tasks/info/<id>
  else if(req.indexOf("GET /tasks/info/")>=0){
    int idx=req.substring(17).toInt();
    if(idx>=0 && idx<NUM_TASKS){
      TCB &t=tasks[idx];
      response="{\"id\":"+String(t.id)+",\"pid\":"+String(t.pid)+",\"name\":\""+String(t.name)+"\",\"state\":\""+String(t.state==TASK_RUNNING?"RUNNING":"STOPPED")+"\",\"last_run_ms\":"+String(t.last_run_ms)+",\"next_run_ms\":"+String(t.next_run_ms)+"}";
    }else response="{\"error\":\"invalid id\"}";
  }
  else response="{\"error\":\"unknown endpoint\"}";

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();
  client.println(response);
}

// ---------- Setup & Loop ----------
void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("Access Point Web Server");

  pinMode(LED_PIN, OUTPUT);      // set the LED pin mode

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  // by default the local IP address will be 192.168.4.1
  // you can override it with the following:
  WiFi.config(IPAddress(192,48,56,2));

  // print the network name (SSID);
  Serial.print("Creating access point named: ");
  Serial.println(ssid);

  // Create open network. Change this line if you want to create an WEP network:
  status = WiFi.beginAP(ssid, pass);
  if (status != WL_AP_LISTENING) {
    Serial.println("Creating access point failed");
    // don't continue
    while (true);
  }

  // wait 10 seconds for connection:
  delay(10000);

  // start the web server on port 80
  server.begin();

  // you're connected now, so print out the status
  printWiFiStatus();

  register_task(0,"Blinker",task_blink,DEFAULT_BLINK_MS,false);
  register_task(1,"SerialPrinter",task_printer,DEFAULT_PRINT_MS,false);
}

void loop(){
  WiFiClient client = server.available();
  if(client) handleClient(client);

  scheduler_tick();
}
