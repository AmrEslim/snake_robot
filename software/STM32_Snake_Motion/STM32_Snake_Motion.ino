#include <Servo.h>

// Create servo objects
Servo servos[8];

// Define pins (D3-D10 on Nucleo L432KC)
const int SERVO_PINS[8] = {
    PB0,  // D3
    PB7,  // D4
    PB6,  // D5
    PB1,  // D6
    PA8,  // D7
    PA9,  // D8
    PA10, // D9
    PA11  // D10
}; 

// Optimized motion parameters based on snake locomotion research
const int CENTER_POS = 90;        // Neutral position
const float AMPLITUDE = 35.0;      // Optimal amplitude for lateral undulation
const float FREQUENCY = 0.8;       // Higher frequency for better propulsion
const float PHASE_LAG = 60.0;      // Phase difference (degrees) - approximately 2π/6
const float WAVE_LENGTH = 1.5;     // Number of complete waves along the body
const int DELAY_MS = 10;          // Faster update rate for smoother motion

// For converting degrees to radians
const float RAD_CONV = PI / 180.0;

// Servo direction correction (alternating segments need opposite directions)
const int SERVO_DIRECTION[8] = {1, -1, 1, -1, 1, -1, 1, -1};

void setup() {
  Serial.begin(9600);
  Serial.println("Snake Robot - Optimized Serpentine Motion");
  
  // Initialize all servos
  for (int i = 0; i < 8; i++) {
    servos[i].attach(SERVO_PINS[i]);
    servos[i].write(CENTER_POS);
    delay(100);
  }
  
  // Wait for servos to center
  delay(1000);
}

void loop() {
  static float time = 0.0;
  
  // Update each servo position
  for (int i = 0; i < 8; i++) {
    // Calculate spatial frequency based on desired wavelength
    float spatial_freq = 2.0 * PI * WAVE_LENGTH / 8.0;
    
    // Calculate phase for this segment
    float phase = i * spatial_freq;
    
    // Generate optimized serpentine wave
    float angle = AMPLITUDE * 
                 sin(2.0 * PI * FREQUENCY * time - phase) * 
                 SERVO_DIRECTION[i];
    
    // Apply additional phase lag for propagating wave
    angle *= sin(phase - (PHASE_LAG * RAD_CONV));
    
    // Convert to servo position
    int position = CENTER_POS + (int)angle;
    
    // Constrain position to safe range
    position = constrain(position, CENTER_POS - AMPLITUDE, CENTER_POS + AMPLITUDE);
    
    // Update servo
    servos[i].write(position);
  }
  
  // Increment time
  time += DELAY_MS / 1000.0;
  if (time >= 100000.0) time = 0.0; // Prevent overflow
  
  // Small delay for smooth motion
  delay(DELAY_MS);
}