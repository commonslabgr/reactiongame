import processing.serial.*;

public class Game {
 
  final String start = "START";
  final float START_ANGLE = 0.0f;
  final float TARGET_ANGLE = TWO_PI;
  final float DELTA = PI/15.0f;
  final String end = "PLAY AGAIN";
  
 
  private float current_angle;
  private int clickCount;
  private int startTime;
  private Serial conn;
  // gameState: 0 startScreen, 1 mainScreen, 2 endScreen
  private int gameState;
  
  public Game(Serial c) {
     this.conn = c;
     this.clickCount = 0;
     this.gameState = 0;
  }
  
  public void update() {
    
    switch(gameState) {
      case 0:
        showStartScreen();
        break;
      case 1:
        connRead();
        showMainScreen();
        break;
       case 2:
         showEndScreen();
         break;
    }
  }
  
  private void connRead() {
    if(conn.available()>0) {
      int led = conn.read();
      if(led>=0) {
        clickCount+=1;
      }
    }
  }

  
  public void init() {
    conn.clear();
    if(gameState == 2) {
       gameState = 0;
       
    } else {
      reset();
      this.gameState = 1;      
      conn.write('g');
    }
  }
  
  private void reset() {
     current_angle = 0;
     clickCount = 0;
     startTime = millis();  
  }
  
  private void showStartScreen() {
     background(0);
     noFill();
     strokeWeight(4);
     stroke(0, 220, 0);
     ellipse(width/2, height/2, 350, 350);
     noStroke();
     fill(220);
     ellipse(width/2, height/2, 330, 330);
     float tw = textWidth(start)/2;
     fill(40);
     textSize(64);
     text(start, width/2-tw, height/2+16);
  }
  
  private void showMainScreen() { 
     background(0);
     noFill();
     strokeWeight(4);
     stroke(0, 220, 0);
     
     arc(width/2, height/2, 400, 400, 0.0, current_angle);
     noStroke();
     fill(220);
     ellipse(width/2, height/2, 380, 380);
     fill(0);
     float tw = textWidth(""+clickCount)/2;
     text(clickCount, width/2-tw, height/2+16);
     current_angle = ((millis()-startTime)/1000) * DELTA;
     if(current_angle >= TWO_PI) {
       conn.write('e');
       gameState = 2;
     }
  }
  
  private void showEndScreen() {
     background(0);
     noFill();
     strokeWeight(4);
     stroke(0, 220, 0);
     arc(width/2, height/2, 400, 400, 0.0, current_angle);
     noStroke();
     fill(220);
     ellipse(width/2, height/2, 380, 380);
     fill(0);
     float ccw = textWidth(""+clickCount)/2;
     text(clickCount, width/2-ccw, height/2+16);
     fill(255);
     float tw = textWidth(end)/2;
     text(end, width/2-tw, 100);
  }
}