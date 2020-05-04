int antennaSignal = A1;
unsigned char sig = 0;
unsigned long lastTimestamp = 0, now = 0, dTime = 0, startLastReading = 0, lastTimeInc = 0;
const int sigLength = 5; // how many measure points to take to reduce low/high blips by taking average
bool buffer = false, sigArray[sigLength];
int data[60], position = 0;

int five[] = {2, HIGH}, ten[] = {2, LOW}, fifteen[] = {3, HIGH}, twenty[] = {3, LOW},
to[] = {4, HIGH}, past[] = {4, LOW}, threequarters[] = {5, HIGH}, half[] = {5, LOW};
int hours[][2] = {{6, HIGH}, {6, LOW}, {7, HIGH}, {7, LOW}, {8, HIGH}, {8, LOW}, {9, HIGH},
{9, LOW}, {10, HIGH}, {10, LOW}, {11, HIGH}, {11, LOW}}; // caution: zero-based; hours[0] is one o clock
float ledBrightness = 0.5;

bool hasParsedTime = false;
int timeParsed[] = {0, 0}, timeNow[] = {0, 0}; // hours, minutes

char debugMode = 2; // options: 0-> nothing, 1-> text, 2-> plot


void setup() {
  if (debugMode != 0){
    Serial.begin(9600);
  }
  pinMode(antennaSignal, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  testClusters();

  // initialize sig
  for (int i = 0; i < sigLength; i++){
    sigArray[i] = false;
  }
}

void loop(){
  now = millis();
  sig = analogRead(antennaSignal);

  if (debugMode == 2){
    Serial.println(sig);
  }
  
  pushSig(sig < 100);
  bool sigBool = getSig();

  // show signal on debugging light
  digitalWrite(LED_BUILTIN, sigBool);

  if(buffer != sigBool)  {
    dTime = now - lastTimestamp;
    lastTimestamp = now;
    if (sigBool) { // step up
      if (dTime > 1500) { // new Minute marker
        parseData();
        position = 0;
      }
    }
    else{  // step down
      int dataBit = dTime > 150 ? 1 : 0;  // bit length
      data[position] = dataBit;

      if (debugMode == 1){
        Serial.print(position);
        Serial.print("->");
        Serial.println(dataBit);
      }
      
      if (position == 58){ // the 59th (zero-based : 58) stepdown is the last one; calc time here
        parseData();
        position = -1; // because it gets incremented right afer
      }

      position++;
    }
  }
  buffer = sigBool;

  if(now - lastTimeInc > 5000){ // check for/show time increment every 5 seconds
    incrementTime(now);
    lastTimeInc = now;
    showTime();
  }
}

void parseData(){ // multiply / add up all the relevant bits to calculate time
  if ((data[21] + data[22] + data[23] + data[24] + data[25] + data[26] + data[27] + data[28]) % 2 == 0 &&  // parity minutes
    (data[29] + data[30] + data[31] + data[32] + data[33] + data[34] + data[35]) % 2 == 0){ // parity hours
    int hoursParsed = data[29]*1 + data[30]*2 + data[31]*4 + data[32]*8 + data[33]*10 + data[34]*20;
    int minsParsed = data[21]*1 + data[22]*2 + data[23]*4 + data[24]*8 + data[25]*10 + data[26]*20 + data[27]*40;

    if (hoursParsed != 0 || minsParsed != 0){
      timeParsed[0] = hoursParsed;
      timeParsed[1] = minsParsed;
      timeNow[0] = hoursParsed;
      timeNow[1] = minsParsed;
        
      
      hasParsedTime = true;
      startLastReading = millis();
    }
    if (debugMode == 1){
      Serial.print("parse:");
      Serial.print(hoursParsed);
      Serial.print(":");
      Serial.println(minsParsed);
    }
  }
  else {
    if (debugMode == 1){
      Serial.println("parity fail");
    }
  }
}

void pushSig(bool value){
  for (int i = sigLength - 1; i > 0; i--){  // move sigArray one over
    sigArray[i] = sigArray[i - 1];
  }

  sigArray[0] = value;  // pus new value
}

bool getSig(){  // get most frequent value; so small blips don't get counted
  int trueCount = 0, falseCount = 0;
  for (int i = 0; i < sigLength; i++){
    if (sigArray[i]){
      trueCount++;
    }
    else{
      falseCount++;
    }
  }

  return trueCount > falseCount;
}

void incrementTime(unsigned long currentMillis){ // increment time from last known time measurement, if no signal is found
  if (hasParsedTime) {
    unsigned long timeSinceParse = currentMillis - startLastReading;
    int minsSinceParse = timeSinceParse / 60000; // 1000ms * 60s
    int minsNow = timeParsed[1] + minsSinceParse;
  
    timeNow[0] = (timeParsed[0] + minsNow / 60) % 24;
    timeNow[1] = minsNow % 60;

    if (debugMode == 1){
      Serial.print("mins since:");
      Serial.println(minsSinceParse);
      Serial.print("timeNow:");
      Serial.print(timeNow[0]);
      Serial.print(":");
      Serial.println(timeNow[1]);
    }
  }
}

// display code
void testClusters(){
  for (int i = 0; i <= 11; i++) {
    showCluster(new int[2] {i, HIGH});
    delay(100);
    showCluster(new int[2] {i, LOW});
    delay(100);
    clean();
  }
}

void clean(){
  for(int d=0;d<=11;) pinMode(d++,INPUT);
}

void showCluster(int cluster[2]){
  pinMode(cluster[0], OUTPUT);
  digitalWrite(cluster[0], cluster[1]);
}

void showTime(){
  int hourIndex = timeNow[0];
  int minRounded = roundTo(timeNow[1], 5);

  // show values
  clean();

  switch(minRounded) {
    case 5:
      showCluster(five);
      showCluster(past);
      break;
    case 10:
      showCluster(ten);
      showCluster(past);
      break;
    case 15:
      showCluster(fifteen);
      showCluster(past);
      break;
    case 20: 
      showCluster(twenty);
      showCluster(past);
      break;
    case 25:
      showCluster(five);
      showCluster(to);
      showCluster(half);
      hourIndex++;  // from here times are in relation to next hour
      break;
    case 30:
      showCluster(half);
      hourIndex++;
      break;
    case 35:
      showCluster(five);
      showCluster(past);
      showCluster(half);
      hourIndex++;
      break;
    case 40:
      showCluster(twenty);
      showCluster(to);
      hourIndex++;
      break;
    case 45:
      showCluster(threequarters);
      hourIndex++;
      break;
    case 50:
      showCluster(ten);
      showCluster(to);
      hourIndex++;
      break;
    case 55:
      showCluster(five);
      showCluster(to);
      hourIndex++;
      break;
    case 60:
      hourIndex++;
      break;
  }

  hourIndex = (hourIndex - 1) % 12;
  showCluster(hours[hourIndex]);
}

int roundTo(int num, int stepSize){
  return num % stepSize > stepSize / 2 ? 
    num + stepSize - num % stepSize :
    num - num % stepSize;
}

