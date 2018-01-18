import controlP5.*;

ControlP5 cp5;
MainPanel mainPanel;
MenuBar topMenu;
GraphBar[] tplotArray;

int numAFEs = 4, numChannels = 4;
int totalCh = numAFEs * numChannels;
int absoluteCount;
float calcVal;

class MainPanel {

  MainPanel(SenseUI wClass) {

    cp5 = new ControlP5(wClass);
    cp5.setAutoDraw(false);

    topMenu = new MenuBar();

    absoluteCount = 0;

    tplotArray = new GraphBar[numAFEs*numChannels];

    for (int i = 1; i < numAFEs+1; i++) {
      for (int j = 1; j < numChannels+1; j++) {
        tplotArray[absoluteCount] = new GraphBar(i, j, 100, 75+absoluteCount * 55, 800, 50, 1);
        absoluteCount++;
      }
    }

    println("Initialized main panel");
  }


  void draw() {
    topMenu.draw();
    cp5.draw();
    if (RECORDING) {
      fileOutput.print(frameCount + " ");
    }
    if (DATAIN) {
      for (int i = 0; i < totalCh; i++) {
        calcVal = float(sampleArray[i]);
        if (RECORDING) {
          fileOutput.print(calcVal + " ");
        }
        tplotArray[i].draw(calcVal);
      }
    }
    if (RECORDING) {
      fileOutput.println();
    }
  }
}