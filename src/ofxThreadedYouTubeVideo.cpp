#include "ofxThreadedYouTubeVideo.h"

ofxThreadedYouTubeVideo::ofxThreadedYouTubeVideo()
{
    startThread();
}

ofxThreadedYouTubeVideo::~ofxThreadedYouTubeVideo()
{
    urls_to_load.close();
    waitForThread(true);
}


// Load a url
//--------------------------------------------------------------
void ofxThreadedYouTubeVideo::loadYouTubeURL(string _url, int _id)
{
	ofYouTubeLoaderEntry entry(_url, _id);

    urls_to_load.send(entry);

}

//------------------------------------------------------------------------------
const string ofxThreadedYouTubeVideo::genRandomString(const int len) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    string s;

    for (int i = 0; i < len; ++i) {
        char c = alphanum[rand() % (sizeof(alphanum) - 1)];
        s.push_back(c);
    }

    return s;
}

//------------------------------------------------------------------------------
const string ofxThreadedYouTubeVideo::getRandomURL()
{
    string url = "";

    const string search_url = "https://www.googleapis.com/youtube/v3/search?q=\"v="+genRandomString(4)+"\"&key=AIzaSyDGYA7woinSUM_eQStrJWgLaCA5fugJ3IA&part=snippet&maxResults=50&";

    cout << "--------------------------------------------------" << endl;
    cout << "URL=" << search_url << endl;

	if (!response.open(search_url)) {
		cout  << "Failed to parse JSON\n" << endl;
	}

    unsigned int numVideos = response["items"].size();
    unsigned int totalVideos = response["pageInfo"]["totalResults"].asUInt();
    ofLogNotice("ofxThreadedYouTubeVideo") << "Total videos = " << totalVideos;
    ofLogNotice("ofxThreadedYouTubeVideo") << "numVideos = " << numVideos;

    if(numVideos == 0) {
        ofLogError("ofxThreadedYouTubeVideo") << "No videos returned";
        return "";
    }

    int i = ofRandom(0,numVideos);

    Json::Value entryNode = response["items"][i];

    cout << "title = " << entryNode["snippet"]["title"].asString() << endl;
    cout << "video id = " << entryNode["id"]["videoId"].asString() << endl;
    cout << "--------------------------------------------------" << endl;

    url = "https://www.youtube.com/watch?v="+entryNode["id"]["videoId"].asString();

    return url;
}

// Get a new url - called from within thread
//--------------------------------------------------------------
bool ofxThreadedYouTubeVideo::getNewURL(ofYouTubeLoaderEntry& entry )
{
    string new_url = entry.input_url;

    if(new_url == "") {
        new_url = getRandomURL();
        if(new_url == "") return false;
    }

    string video_url = "youtube-dl -g -f 18 \"" + new_url + "\"";

	FILE *in;
	char buff[2048];

	if(!(in = popen(video_url.c_str(), "r"))){
		cout << "failed to popen" << endl;
		return false;
	}

	while(fgets(buff, sizeof(buff), in)!=NULL){
		;//cout << buff;
	}
	pclose(in);

    entry.bLoaded = false;
    video_url = buff;
    video_url.erase( std::remove_if(video_url.begin(), video_url.end(), ::isspace ), video_url.end() );
    //video_url = "\"" + video_url + "\"";

    entry.url = video_url;
    return true;

}

void ofxThreadedYouTubeVideo::threadedFunction()
{
    setThreadName("ofxThreadedImageLoader " + ofToString(thread.get_id()));

    ofLogVerbose("ofxThreadedImageLoader") << "finishing thread on closed queue";

	while( isThreadRunning() ) {

       ofYouTubeLoaderEntry entry;
       while( urls_to_load.receive(entry) ) {

            if(!getNewURL(entry)) {
                ofLogError("ofxThreadedYouTubeVideo") << "couldn't load url: \"" << entry.input_url << "\"";
                //get another random video and try again
                loadYouTubeURL("",entry.id);
            }
            else {
                cout << "ofxThreadedYouTubeVideo got video url: " << entry.url << endl;
                ofVideoPlayer* vid = new ofVideoPlayer();
                vid->setUseTexture(false);
                vid->load(entry.url);
                ofxYouTubeURLEvent e = ofxYouTubeURLEvent(entry.url, entry.id,vid);
                ofNotifyEvent(youTubeURLEvent, e, this);
            }



        }

    } //is thread running
}
