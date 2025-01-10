#include <Servo.h>
#include <math.h>

const int BUZZ_PIN   = 8;
const int TRIG_PIN   = 9;
const int ECHO_PIN   = 10;
const int SERVO_PIN  = 11;
const int BUFFSIZE   = 20;
const float T        = 3.883; // One-side T-test critical value (dof:19, 99.5%CI)

typedef struct CI {
  float low;
  float high;
}; 

float distance;
CI base[3];
float sds[] = {0.0, 0.0, 0.0};
float distbuff[BUFFSIZE];
int angles[] = {45, 90, 135};
int prevangleindex;
int actualangleindex;
Servo motor;

/* Waiting to leave the area */
void prefetch(){
  int i,j;
  for(i = 0; i < 6; i++){
    for(j = 0; j < i; j++){
      tone(BUZZ_PIN, 500);
      delay(300);
      noTone(BUZZ_PIN);
      delay(300);
    }
    delay(500);
  }
}

/* Measure distance */
float get_distance() {
  float time;
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(5);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  time = pulseIn(ECHO_PIN, HIGH);
  return(time * 0.0343 / 2.0);
}

void smooth_step(int startpos, int endpos) {
  int i, step;

  if(endpos < startpos){
    for(i = startpos; i > endpos; i--){
      motor.write(i);
      delay(20);
    }
  } else {
    for(i = startpos; i < endpos; i++){
      motor.write(i);
      delay(20);
    }
  }
}

void collect_dists(){
  int i;
  for(i = 0; i < BUFFSIZE; i++){
    distbuff[i] = get_distance();
  }
}

float calc_avg(){
  int i;
  float sum = 0.0;
  for(i = 0; i < BUFFSIZE; i++){
    sum = sum + distbuff[i];
  }
  return sum / (float)BUFFSIZE;
}

float calc_sd(float avg){
  int i;
  float sum = 0.0;
  for(i = 0; i < BUFFSIZE; i++){
    sum = sum + (distbuff[i] - avg) * (distbuff[i] - avg);
  }
  return sqrt(sum / (float)BUFFSIZE);
}

void collect_baseline(int orientation){
  float avg;
  float sd;
  collect_dists();
  avg = calc_avg();
  sd = calc_sd(avg);
  base[orientation].low = avg - T * (sd / sqrt(20.0));
  base[orientation].high = avg + T * (sd / sqrt(20.0));
}

void initialize_distances(){
  smooth_step(angles[1], angles[0]);
  collect_baseline(0);
  smooth_step(angles[0], angles[1]);
  collect_baseline(1);
  smooth_step(angles[1], angles[2]);
  collect_baseline(2);
  smooth_step(angles[2], angles[1]);
}

bool is_extreme_dist(int orientation){
  float avg;

  collect_dists();
  avg = calc_avg();

  if(base[orientation].low < avg && base[orientation].high > avg){
    return false;
  }

  Serial.print(orientation);
  Serial.print(": ");
  Serial.print(base[orientation].low);
  Serial.print(" ");
  Serial.print(base[orientation].high);
  Serial.print(" ");
  Serial.print(avg);
  Serial.println();

  return true;
}

void setup() {
  pinMode(ECHO_PIN, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(BUZZ_PIN, OUTPUT);
  motor.attach(SERVO_PIN);

  digitalWrite(TRIG_PIN, LOW);
  digitalWrite(BUZZ_PIN, LOW);

  Serial.begin(9600);
  motor.write(90); // set to middle position
  prevangleindex = 1;
  prefetch(); // waiting ~5 sec to leave the area
  initialize_distances(); // collecting initial distances
}

void loop() {
  actualangleindex = random(3);
  smooth_step(angles[prevangleindex], angles[actualangleindex]);
  if(is_extreme_dist(actualangleindex) == true){
    tone(BUZZ_PIN, 300);
  } else {
    noTone(BUZZ_PIN);
  }
  prevangleindex = actualangleindex;
}
