#include "ofApp.h"
#include <array>

# define M_PI 3.14159265358979323846

using namespace hoa;

//--------------------------------------------------------------
void ofApp::setup() {
    
    listener = shared_ptr<Draggable>(new Draggable(glm::vec3(ofGetWidth()/2.f, ofGetHeight()/2.f, 0)));

    beat.openFile(ofToDataPath("soundscape1.wav", true));
    stk::Stk::setSampleRate(44100.0);
    //    uncomment this line to print avaliable audio devices
    //ofSoundStreamListDevices();

    //soundStream.setDeviceId(5); // use this function to set the audio device if necessary
    
    
    //number of outputs must be >= order*2+1 for decoder on regular mode*/
    nOutputs = 2;
    nInputs = 0;
    sampleRate = 48000;
    bufferSize = 1024;
    nBuffers = (nInputs + nOutputs) * 2;
    
    // initialize input to 0
    input = 0;
  
    order = 3;
    // create the spherical harmonics buffer, it must have order*2+1 values for
    // 2 dimensions
    harmonicsBuffer.resize(order * 2 + 1);
    
    // the encoder calculates the spherical harmonics
    hoaEncoder = make_unique<Encoder<Hoa2d, float>::DC>(order);
    
    /* the decoder translates the harmonics into audio signals for output.
     the number of minimum ouput channels for regular mode = order*2+1
     smaller values may be used, but the resulting sound won't be as expected
     for small differences ( 5 or 6 instead of 7 speakers) irregular mode may
     be used */
    hoaDecoder = make_unique<Decoder<Hoa2d, float>::Regular>(order, nOutputs);
    
    // binaural mode set for use with headphones
    //    hoaDecoder = new Decoder<Hoa2d, float>::Binaural(order);
    
    /* rendering is computed in relation to the speaker's angles
     they maybe set with the function hoaDecoder->setPlanewaveAzimuth(const
     ulong index,
     const float azimuth); */
    //hoaDecoder->setPlanewaveAzimuth(0, 0.5);
    hoaDecoder->computeRendering(bufferSize);
    
    /*the optim allows to acount for displacements in ideal speaker position
     "basic" works as a bypass.
     "inphase" gand "maxre" should be used if the ambsionics circle/sphere is
     not perfect */
    hoaOptim = make_unique<Optim<Hoa2d, float>::Basic>(order);
    //    hoaOptim = new Optim<Hoa2d, float>::InPhase(order);
    
    // ofxHoaCoord used to set source position and void clicks in audio
    hoaCoord = make_unique<ofxHoaCoord<Hoa2d, float>>(1);
    
    // set the position in screen that'll represent the center of the speaker
    // circle and it's radius
    circleCenter = ofVec3f(ofGetWidth() / 2, ofGetHeight() / 2);
    circleRadius = 50;
    hoaCoord->setAmbisonicCenter(circleCenter);
    hoaCoord->setAmbisonicRadius(circleRadius);
    
    // set the ramp for interpolation in milliseconds
    hoaCoord->setRamp(50, sampleRate);
    
    // functions to set the position of the encoded sound source
    // far away to avoid clicks in the begining
    hoaCoord->setSourcePositionDirect(0, ofVec3f(10000, 10000)); //INITIAL
    //hoaCoord->setSourcePositionDirect(0, position[i]);
    
    // make a prettier circle
    //ofSetCircleResolution(50);
    
    // initialize soundstream
    soundStream.setup(this, nOutputs, nInputs, sampleRate, bufferSize,
                      nBuffers);
    
    
}

//--------------------------------------------------------------
void ofApp::update() {
    // change source position
    listener->update();
    //sourcePosition = ofVec3f(mouseX, mouseY);
    _sourcePosition = ofVec3f(listener->pos.x, listener->pos.y); //not this
    hoaCoord->setSourcePosition(0, _sourcePosition);
}

//--------------------------------------------------------------
void ofApp::draw() {
    //ofBackgroundGradient(ofColor::gold, ofColor::black);
    ofBackground(30,30,30);
    ofSetColor(ofColor::crimson);
    ofFill();
    ofDrawCircle(_sourcePosition, 10);
    ofNoFill();
    ofDrawCircle(circleCenter, circleRadius);
    
    ofDrawBitmapString(ofSoundStreamListDevices(), 100, 100);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
    cout << hoaCoord->getAzimuth(0);
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key) {}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y) {}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {
    listener->mousePressed(x, y, button);

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button) {
    listener->mouseReleased(x, y, button);
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h) {}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg) {}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo) {}

void ofApp::audioOut(float* output, int bufferSize, int nChannels) {
    for (int i = 0; i < bufferSize; i++) {
        
        
        input = beat.tick();
        
        hoaCoord->process();
        
        // create audio input. the last multiplication is the volume (should be
        // between 0 and 1)
        //input = myOsc.tick() * (myEnv.tick() + 1) * 0.4;
        // set smoothed current radius and azimuth
        hoaEncoder->setRadius(hoaCoord->getRadius(0));
        hoaEncoder->setAzimuth(hoaCoord->getAzimuth(0) + (M_PI+(M_PI/2))); //here set the azimuth
        
        // create the spherical harmonics
        hoaEncoder->process(&input, &harmonicsBuffer[0]);
        
        // process the harmonics with optim
        hoaOptim->process(&harmonicsBuffer[0], &harmonicsBuffer[0]);
        
        // decode the harmonics; audio treatements are possible in between these
        // steps
        hoaDecoder->process(&harmonicsBuffer[0], output + i * nChannels);
    }
}

void ofApp::exit() {
    /*
     // close soundStream
     soundStream.close();
     
     delete [] outputMatrix[0];
     delete [] outputMatrix[1];
     delete [] outputMatrix;
     
     for (int i = 0; i<order*2 + 1; ++i) delete[] harmonicMatrix[i];
     delete[] harmonicMatrix;
     */
}


