#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    chessboardSize = 300;
    chessboardX = 5;
    chessboardY = 4;
    
    kinect.setup();
    kinect.addImageGenerator();
    kinect.addDepthGenerator();
    kinect.setRegister(true);
    kinect.setMirror(false);
    kinect.setUseDepthRawPixels(true);
    kinect.setDepthColoring(COLORING_GREY);
    kinect.start();
    
    rgbImage.allocate(kinect.getWidth(), kinect.getHeight());
    
    fboChessboard.allocate(PROJECTOR_RESOLUTION_X, PROJECTOR_RESOLUTION_Y, GL_RGBA);
    secondWindow.setup("chessboard", ofGetScreenWidth(), 0, fboChessboard.getWidth(), fboChessboard.getHeight(), true);
}

//--------------------------------------------------------------
void ofApp::drawChessboard(int x, int y, int chessboardSize) {
    float w = chessboardSize / chessboardX;
    float h = chessboardSize / chessboardY;
    currentProjectorPoints.clear();
    fboChessboard.begin();
    ofBackground(255);
    ofSetColor(0);
    ofTranslate(x, y);
    for (int j=0; j<chessboardY; j++) {
        for (int i=0; i<chessboardX; i++) {
            int x0 = ofMap(i, 0, chessboardX, 0, chessboardSize);
            int y0 = ofMap(j, 0, chessboardY, 0, chessboardSize);
            if (j>0 && i>0) {
                currentProjectorPoints.push_back(ofVec2f(
                                                         ofMap(x+x0, 0, fboChessboard.getWidth(), 0, 1),
                                                         ofMap(y+y0, 0, fboChessboard.getHeight(), 0, 1)
                                                         ));
            }
            if ((i+j)%2==0) ofRect(x0, y0, w, h);
        }
    }
    ofSetColor(255);
    fboChessboard.end();
}

//--------------------------------------------------------------
void ofApp::drawTestingPoint(ofVec2f projectedPoint) {
    float ptSize = ofMap(sin(ofGetFrameNum()*0.1), -1, 1, 3, 40);
    fboChessboard.begin();
    ofBackground(255);
    ofSetColor(0, 255, 0);
    ofCircle(
             ofMap(projectedPoint.x, 0, 1, 0, fboChessboard.getWidth()),
             ofMap(projectedPoint.y, 0, 1, 0, fboChessboard.getHeight()),
             ptSize);
    ofSetColor(255);
    fboChessboard.end();
}

//--------------------------------------------------------------
void ofApp::addPointPair() {
    
    int nDepthPoints = 0;
    for (int i=0; i<cvPoints.size(); i++) {
        ofPoint depthPoint = ofPoint((int)cvPoints[i].x, (int)cvPoints[i].y, depthPixels[(int)cvPoints[i].x + (int)cvPoints[i].y * kinect.getWidth()]);
        ofVec3f worldPoint = kinect.projectiveToWorld(depthPoint);
        if (worldPoint.z > 0)   nDepthPoints++;
    }
    if (nDepthPoints == (chessboardX-1)*(chessboardY-1)) {
        for (int i=0; i<cvPoints.size(); i++) {
            ofPoint depthPoint = ofPoint((int)cvPoints[i].x, (int)cvPoints[i].y, depthPixels[(int)cvPoints[i].x + (int)cvPoints[i].y * kinect.getWidth()]);
            ofVec3f worldPoint = kinect.projectiveToWorld(depthPoint);
            pairsKinect.push_back(worldPoint);
            pairsProjector.push_back(currentProjectorPoints[i]);
        }
        resultMessage = "Added " + ofToString((chessboardX-1)*(chessboardY-1)) + " points pairs.";
        resultMessageColor = ofColor(0, 255, 0);
    } else {
        resultMessage = "Points not added because not all chessboard\npoints' depth known. Try re-positionining.";
        resultMessageColor = ofColor(255, 0, 0);
    }
}

//--------------------------------------------------------------
void ofApp::update(){
    kinect.update();
    if (kinect.isNewFrame()) {
        depthPixels = kinect.getDepthRawPixels();
        rgbImage.setFromPixels(kinect.getImagePixels());
        if (testing) {
            ofVec2f t = ofVec2f(min(kinect.getWidth()-1,testPoint.x), min(kinect.getHeight()-1,testPoint.y));
            ofPoint depthPoint = ofPoint(t.x, t.y, depthPixels[t.x + t.y * kinect.getWidth()]);
            ofVec3f worldPoint = kinect.projectiveToWorld(depthPoint);
            ofVec2f projectedPoint = kpt.getProjectedPoint(worldPoint);
            drawTestingPoint(projectedPoint);
        }
        else {
            drawChessboard(ofGetMouseX(), ofGetMouseY(), chessboardSize);
            cvRgbImage = ofxCv::toCv(rgbImage.getPixelsRef());
            cv::Size patternSize = cv::Size(chessboardX-1, chessboardY-1);
            int chessFlags = cv::CALIB_CB_ADAPTIVE_THRESH + cv::CALIB_CB_FAST_CHECK;
            foundChessboard = findChessboardCorners(cvRgbImage, patternSize, cvPoints, chessFlags);
            if(foundChessboard) {
                cv::Mat gray;
                cvtColor(cvRgbImage, gray, CV_RGB2GRAY);
                cornerSubPix(gray, cvPoints, cv::Size(11, 11), cv::Size(-1, -1),
                             cv::TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 30, 0.1));
                drawChessboardCorners(cvRgbImage, patternSize, cv::Mat(cvPoints), foundChessboard);
                
                
            }
        }
    }
}

//--------------------------------------------------------------
void ofApp::draw(){
    rgbImage.draw(0, 0);
    kinect.drawDepth(10, 490, 320, 240);
    
    ofSetColor(0);
    if (testing) {
        ofDrawBitmapString("Click on the image to test a point in the RGB image.", 340, 510);
        ofDrawBitmapString("The projector should place a green dot on the corresponding point.", 340, 530);
        ofDrawBitmapString("Press the 's' key to save the calibration.", 340, 550);
        if (saved) {
            ofDrawBitmapString("Calibration saved.", 340, 590);
        }
        ofSetColor(255, 0, 0);
        float ptSize = ofMap(cos(ofGetFrameNum()*0.1), -1, 1, 3, 40);
        ofCircle(testPoint.x, testPoint.y, ptSize);
    } else {
        ofDrawBitmapString("Position the chessboard using the mouse.", 340, 510);
        ofDrawBitmapString("Adjust the size of the chessboard using the 'q' and 'w' keys.", 340, 530);
        ofDrawBitmapString("Press the spacebar to save a set of point pairs.", 340, 550);
        ofDrawBitmapString("Press the 'c' key to calibrate.", 340, 570);
        ofSetColor(resultMessageColor);
        ofDrawBitmapString(resultMessage, 340, 610);
        ofSetColor(0);
        ofDrawBitmapString(ofToString(pairsKinect.size())+" point pairs collected.", 340, 630);
    }
    ofSetColor(255);
    
    secondWindow.begin();
    fboChessboard.draw(0, 0);
    secondWindow.end();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    if (key==' '){
        addPointPair();
    } else if (key=='q') {
        chessboardSize -= 20;
    } else if (key=='w') {
        chessboardSize += 20;
    } else if (key=='c') {
        kpt.calibrate(pairsKinect, pairsProjector);
        testing = true;
    } else if (key=='s') {
        kpt.saveCalibration(ofToDataPath("calibration.xml"));
        saved = true;
    } else if (key=='l') {
        kpt.loadCalibration(ofToDataPath("calibration.xml"));
        testing = true;
    }
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
    if (testing) {
        testPoint.set(min(x, (int)kinect.getWidth()-1), min(y, (int)kinect.getHeight()-1));
    }
}
