// Global variables
let time = 0;
let mode = 'lateral'; // 'lateral' or 'sidewinding'

// Configuration variables
let numSegments = 10;
let segmentSpacing = 40;
let centerY = 300;
let centerX = 500;
let amplitude = 50;
let frequency = 1;
let phaseOffset; // Will be set in setup using p5.js PI

function setup() {
  createCanvas(800, 600);
  
  // Initialize phase offset using p5.js PI constant
  phaseOffset = PI / 3;
  
  // Create mode selection button
  let button = createButton('Toggle Mode');
  button.position(10, 10);
  button.mousePressed(() => {
    mode = mode === 'lateral' ? 'sidewinding' : 'lateral';
  });
}

function getSegmentPosition(index) {
  if (mode === 'lateral') {
    // Lateral undulation
    const x = centerX - index * segmentSpacing;
    const y = centerY + amplitude * sin(frequency * time + index * phaseOffset);
    return { x, y };
  } else {
    // Sidewinding
    const x = centerX - index * segmentSpacing + 
              amplitude * sin(frequency * time + index * phaseOffset);
    const y = centerY + 
              amplitude * cos(frequency * time + index * phaseOffset);
    return { x, y };
  }
}

function draw() {
  background(240);
  
  // Draw title
  textSize(20);
  textAlign(CENTER);
  fill(0);
  text(`Snake Robot ${mode === 'lateral' ? 'Lateral Undulation' : 'Sidewinding'}`, width/2, 30);
  
  // Instructions
  textSize(14);
  textAlign(LEFT);
  text('Click button to toggle between modes', 10, 50);
  
  // Draw ground reference line
  stroke(100);
  strokeWeight(1);
  line(0, centerY + 100, width, centerY + 100);
  
  // Draw snake segments
  for (let i = 0; i < numSegments; i++) {
    const pos = getSegmentPosition(i);
    const nextPos = i < numSegments - 1 ? getSegmentPosition(i + 1) : null;
    
    // Draw connection line to next segment
    if (nextPos) {
      stroke(100);
      strokeWeight(2);
      line(pos.x, pos.y, nextPos.x, nextPos.y);
    }
    
    // Draw segment
    noStroke();
    fill(i === 0 ? color(255, 0, 0) : color(0, 0, 255));
    circle(pos.x, pos.y, 16);
  }
  
  // Draw movement trail
  const headPos = getSegmentPosition(0);
  if (frameCount % 5 === 0) {
    noStroke();
    fill(255, 0, 0, 50);
    circle(headPos.x, headPos.y, 8);
  }
  
  // Update time
  time += 0.05;
  
  // Draw legend
  drawLegend();
}

function drawLegend() {
  const legendX = 20;
  const legendY = height - 80;
  
  textAlign(LEFT);
  textSize(14);
  fill(0);
  text('Legend:', legendX, legendY);
  
  // Head marker
  fill(255, 0, 0);
  circle(legendX + 20, legendY + 20, 16);
  fill(0);
  text('Head', legendX + 40, legendY + 25);
  
  // Body marker
  fill(0, 0, 255);
  circle(legendX + 20, legendY + 45, 16);
  fill(0);
  text('Body Segment', legendX + 40, legendY + 50);
}