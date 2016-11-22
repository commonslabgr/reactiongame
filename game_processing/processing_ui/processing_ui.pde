import processing.serial.*;

Serial conn;
Game g;

void setup() {
  size(800, 600);
  //fullScreen();
  String[] portNames = Serial.list();
  conn = new Serial(this, portNames[0], 9600);
  g = new Game(conn);
}

void draw() {
  g.update();
  //g.render();
}

void mousePressed() {
  g.init();
}