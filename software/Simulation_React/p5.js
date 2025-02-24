let numSegments = 10;
let segmentLength = 30;
let mode = 0; // 0 = lateral undulation, 1 = sidewinding

// Parameters similar to your robot
let amplitude = 30;       // Horizontal amplitude (in pixels)
let frequency = 1;        // Frequency multiplier
let phaseOffset = 60;     // Phase offset (degrees)
let verticalAmplitude = 20;    // Vertical amplitude for sidewinding (in pixels)
let verticalPhaseOffset = 90;  // Additional phase offset for vertical movement

function setup() {
  createCanvas(800, 400);
  angleMode(DEGREES);
  frameRate(60);
  
  // Create a button to toggle between modes
  let modeButton = createButton('Toggle Mode');
  modeButton.position(10, 10);
  modeButton.mousePressed(toggleMode);
}

function toggleMode() {
  mode = (mode + 1) % 2;
}

function draw() {
  background(240);
  let time = millis() * 0.001 * frequency * 360; // time in degrees
  
  // Starting position for the snake’s head
  let startX = 100;
  let startY = height / 2;
  
  // Array to store the positions of each segment
  let positions = [];
  
  // Compute the position of each segment based on the mode
  for (let i = 0; i < numSegments; i++) {
    let segmentPhase = i * phaseOffset;
    let x, y;
    
    if (mode === 0) { 
      // Lateral Undulation: Only horizontal sine offsets affect vertical position.
      x = startX + i * segmentLength;
      y = startY + amplitude * sin(time + segmentPhase);
    } else {
      // Sidewinding: Both horizontal and vertical sine functions drive the movement.
      let verticalPhase = verticalPhaseOffset + i * phaseOffset;
      x = startX + i * segmentLength + amplitude * sin(time + segmentPhase);
      y = startY + verticalAmplitude * sin(time + verticalPhase);
    }
    
    positions.push(createVector(x, y));
  }
  
  // Draw the snake segments and connect them with lines
  stroke(0);
  strokeWeight(2);
  fill(100, 200, 100);
  for (let i = 0; i < positions.length; i++) {
    ellipse(positions[i].x, positions[i].y, 15, 15);
    if (i > 0) {
      line(positions[i - 1].x, positions[i - 1].y, positions[i].x, positions[i].y);
    }
  }
  
  // Display the current mode on the canvas
  noStroke();
  fill(0);
  textSize(16);
  text("Mode: " + (mode === 0 ? "Lateral Undulation" : "Sidewinding"), 10, height - 20);
}
