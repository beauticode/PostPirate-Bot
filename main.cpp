/*@PostPirateBot developed by BeautiCode (aka DIYGary)
 * Gary S - Copyright (c) 2017
 * All Rights Reserved
 * realbeauticode@gmail.com
 */
/*@PostPirateBot is an open-source, free to view, twitter bot written in C++.
 * The bot searches for and re-posts the latest tweets that
 * contain the latest top-trending term in a specific region.
 */
/*This program uses the following libraries:
 * libcurl
 * nlohmann's JSON
 * Portable C++ Hashing Library
 * CPPCodec base64 library
 */

#include <iostream>
#include <string>
#include "json.hpp"
#include "hmac.h"
#include "sha1.h"
#include <curl/curl.h>
#include "cppcodec/base32_default_crockford.hpp"
#include "cppcodec/base64_default_rfc4648.hpp"
CURLcode getBearerToken(std::string* keyvar); //Used for authencation when searching for the current trend; Requests bearer token from API
CURLcode getCurrentTrend(std::string* trendvar); //Sends API request to get a list of the current trends in a specific region, then parses out the first.
CURLcode getTrendTweet(std::string trend, std::string* tweetvar); //Sends API request to search for tweets containing the current trend, then parses out the first.
CURLcode postTrendTweet(std::string trendTweet); //Sends an API request to update the bot's twitter feed with the latest tweet regarding the latest trend (Without mentioning the original author)
std::string parseTrendTweet(std::string trendTweet); //Parses out the tweet's reference to it's original author or any other user.
std::string hexStringtoASCII(std::string hexString); //Converts a string of hex values (from the HMAC-SHA1 function) into a string that contains the characters representative of those bytes. This is important for correct signature calculation.
std::string getRandomString(); //Produces a random string for the nonce calculation.
size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata); //Function needed by libcurl in order to read response data from a host.
std::time_t time0=0,time1=0; // Time1 - TimeNought = time in between
float version=1.0; //@PostPirateBot version
int delay=900; //Time between each tweet, in seconds.
long woeID=23424977; //Where On Earth ID; where do we search for the top-trending terms?
struct authencation { //Parameters needed to successfully authencate bot; can be obtained from apps.twitter.com
    /*std::string user="PostPirateBot";
    std::string pass="(removed)";
    std::string consumerKey="(removed)";
    std::string consumerSecret="(removed)";
    std::string accessToken="(removed)";
    std::string accessSecret="(removed)";*/
    std::string user="PostPirateBot";
    std::string pass="3494279";
    std::string consumerKey="DADw7W9majB5gnDuSCsFFeyPL";
    std::string consumerSecret="fWwkjSaHEeCdvwOYZVUlIjO9uoVHrGR3CBbRBVzhFCt8BowUut";
    std::string accessToken="869274271155011585-pRNc6d0NWjQWe5091qFBc3ksM00KVM3";
    std::string accessSecret="7XdKp0PCLr5oXqHnBSLeY26nAcYpeVbTTPNnfDAj1GDB1";    
    std::string encodedAuth= consumerKey+":"+consumerSecret; //Base64 encoded consumerKey:consumerSecret
};
std::string accessKey="",oAuthSecret="",bearerToken="";
std::string responseData="",oAuthToken="",oAuthAccessKey="";
std::string previousTrendTweet="";
bool verbose=false;
int main() {
    short stop1;
    std::cout<<"Program running.\n";
    if(true) { //Ignore this.
        std::cout<<"Obtaining bearer token.\n";
        stop1=getBearerToken(&accessKey);
        while(!stop1) { //As long as obtaining the bearer token was successful:
            if((std::time(NULL) - time0)>=(delay)) {//Get current time and check if 15 minutes have passed since last time
                std::string currentTrend="",currentTrendTweet="";
                if(!getCurrentTrend(&currentTrend)) {
                    std::cout<<"Trend:"<<currentTrend<<std::endl;
                    if(!getTrendTweet(currentTrend,&currentTrendTweet)) {
                        std::cout<<"Trend Tweet:"<<currentTrendTweet<<std::endl;
                        if(currentTrendTweet!=previousTrendTweet) {
                            if(!postTrendTweet(currentTrendTweet)) {
                                std::cout<<"Posted a new status!\n";
                                previousTrendTweet=currentTrendTweet;
                            } else {
                                std::cout<<"Could not post, enable verbose.\n";
                            }
                        } else {
                            std::cout<<"Duplcate tweet error! (No new tweets)\n";
                            if(!postTrendTweet("No new tweets for me to post. Standby!")) {
                                std::cout<<"Posted a new status!\n";
                            } else {
                                std::cout<<"Could not post, enable verbose.\n";
                            }
                        }
                        currentTrendTweet.clear();
                    }
                }
                time0 = std::time(NULL); //Reset timer
            }
        }
    }
    std::cout<<"End result: "<<stop1<<std::endl;
} 

CURLcode getBearerToken(std::string* keyvar) {
    CURL* active=curl_easy_init();
    struct curl_slist *headers=NULL;
    authencation credentials;
    std::string response;
    curl_slist_append(headers,"Content-Type: application/x-www-form-urlencoded;charset=UTF-8");
    curl_easy_setopt(active,CURLOPT_HTTPHEADER,headers);
    curl_easy_setopt(active,CURLOPT_URL,"https://api.twitter.com/oauth2/token");
    curl_easy_setopt(active,CURLOPT_USERAGENT,"PostPirate v"+std::to_string(version));
    curl_easy_setopt(active,CURLOPT_HTTPAUTH,CURLAUTH_BASIC);
    curl_easy_setopt(active,CURLOPT_USERPWD,credentials.encodedAuth.c_str());
    curl_easy_setopt(active,CURLOPT_POSTFIELDS,"grant_type=client_credentials");
    curl_easy_setopt(active,CURLOPT_WRITEFUNCTION,write_callback);
    curl_easy_setopt(active,CURLOPT_VERBOSE,verbose);
    responseData.clear();
    CURLcode response_code=curl_easy_perform(active);
    if(!responseData.empty()) {
        bearerToken=nlohmann::json::parse(responseData)["access_token"];
    }
    curl_easy_cleanup(active);
    return response_code;
}

size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
    responseData+=ptr;
    return size*nmemb;
}

CURLcode getCurrentTrend(std::string* trendvar) {
    struct curl_slist *tokenheader=NULL;
    CURL* active=curl_easy_init();
    std::string tokenheader_string="Authorization: Bearer "+bearerToken;
    tokenheader=curl_slist_append(tokenheader,tokenheader_string.c_str());
    tokenheader=curl_slist_append(tokenheader,"Accept: application/json");
    tokenheader=curl_slist_append(tokenheader,"Accept-Charset: utf-8");
    tokenheader=curl_slist_append(tokenheader,"Accept-Encoding: identity");
    curl_easy_setopt(active,CURLOPT_HTTPGET,1L);
    std::string url=std::string("https://api.twitter.com/1.1/trends/place.json?id=")+std::to_string(woeID);
    curl_easy_setopt(active,CURLOPT_URL,url.c_str());
    curl_easy_setopt(active,CURLOPT_USERAGENT,"PostPirate v"+std::to_string(version));
    curl_easy_setopt(active,CURLOPT_WRITEFUNCTION,write_callback);
    curl_easy_setopt(active,CURLOPT_VERBOSE,verbose);
    curl_easy_setopt(active,CURLOPT_HTTPHEADER,tokenheader);
    responseData.clear();
    CURLcode response_code=curl_easy_perform(active);
    if(!responseData.empty()&&response_code==0) {
        *trendvar=nlohmann::json::parse(responseData.c_str())[0]["trends"][0]["name"];
    }
    curl_easy_cleanup(active);
    return response_code;
}

std::string getRandomString() {
    std::string alphas= "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::string returnVal="";
    for(short x=0;x<32;x++) {
        returnVal.append(1,alphas[rand()%(alphas.length()-1)]);
    }
    return returnVal;
}

CURLcode getTrendTweet(std::string trend, std::string* tweetvar) {
    CURL* active=curl_easy_init();
    authencation authStruct;
    struct curl_slist *tokenheader=NULL;
    std::string requestTime=std::to_string(static_cast<long>(std::time(NULL)));
    std::string randomString=getRandomString();
    tokenheader=curl_slist_append(tokenheader,"Content-Type: application/x-www-form-urlencoded");
    std::string host_baseURL="https://api.twitter.com/1.1/search/tweets.json";
    std::string host=host_baseURL+"?q="+curl_easy_escape(active,trend.c_str(),trend.length());
    std::string host_argument="oauth_consumer_key="+authStruct.consumerKey+"&oauth_nonce="+randomString+"&oauth_signature_method=HMAC-SHA1&oauth_timestamp="+requestTime+"&oauth_token="+authStruct.accessToken+"&oauth_version=1.0&q="+std::string(curl_easy_escape(active,trend.c_str(),trend.length())); 
    std::string signingKey=std::string(curl_easy_escape(active,authStruct.consumerSecret.c_str(),authStruct.consumerSecret.length()))+"&"+std::string(curl_easy_escape(active,authStruct.accessSecret.c_str(),authStruct.accessSecret.length()));
    std::string signatureBase=std::string("GET&")+curl_easy_escape(active,host_baseURL.c_str(),host_baseURL.length())+std::string("&")+curl_easy_escape(active,host_argument.c_str(),host_argument.length());
    std::string signature_=curl_easy_escape(active,base64::encode(hexStringtoASCII(hmac<SHA1>(signatureBase,signingKey).c_str())).c_str(),base64::encode(hexStringtoASCII(hmac<SHA1>(signatureBase,signingKey).c_str())).length());
    std::string oAuthHeader=std::string("Authorization: OAuth oauth_consumer_key=\"")+std::string(curl_easy_escape(active,authStruct.consumerKey.c_str(),authStruct.consumerKey.length()))+std::string("\", oauth_nonce=\""+std::string(curl_easy_escape(active,randomString.c_str(),randomString.length())))+std::string("\", oauth_signature=\"")+signature_+std::string("\", oauth_signature_method=\"HMAC-SHA1\", oauth_timestamp=\"")+requestTime+std::string("\", oauth_token=\"")+std::string(curl_easy_escape(active,authStruct.accessToken.c_str(),authStruct.accessToken.length()))+std::string("\", oauth_version=\"1.0\"");
    tokenheader=curl_slist_append(tokenheader,oAuthHeader.c_str());
    curl_easy_setopt(active,CURLOPT_HTTPGET,1L);
    curl_easy_setopt(active,CURLOPT_URL,host.c_str());
    curl_easy_setopt(active,CURLOPT_USERAGENT,"PostPirate v"+std::to_string(version));
    curl_easy_setopt(active,CURLOPT_WRITEFUNCTION,write_callback);
    curl_easy_setopt(active,CURLOPT_VERBOSE,verbose);
    curl_easy_setopt(active,CURLOPT_HTTPHEADER,tokenheader);
    responseData.clear();
    CURLcode response_code=curl_easy_perform(active);
    if(!responseData.empty()&&response_code==0) {
        *tweetvar=nlohmann::json::parse(responseData.c_str())["statuses"][0]["text"];
    }
    curl_easy_cleanup(active);
    return response_code;
}


std::string hexStringtoASCII(std::string hexString) {
    std::string ASCII;
    for(unsigned short x=0;x<hexString.length();x+=2) {
        char asciiCharacter=(char)(int)strtol(hexString.substr(x,2).c_str(),NULL,16);
        ASCII.push_back(asciiCharacter);
    }
    return ASCII;
}

CURLcode postTrendTweet(std::string trendTweet) {
    trendTweet=parseTrendTweet(trendTweet);
    CURL* active=curl_easy_init();
    authencation authStruct;
    struct curl_slist *tokenheader=NULL;
    std::string requestTime=std::to_string(static_cast<long>(std::time(NULL)));
    std::string randomString=getRandomString();
    tokenheader=curl_slist_append(tokenheader,"Content-Type: application/x-www-form-urlencoded");
    std::string host_baseURL="https://api.twitter.com/1.1/statuses/update.json";
    std::string postStatusData=std::string("status=")+curl_easy_escape(active,trendTweet.c_str(),trendTweet.length());
    std::string host_argument="oauth_consumer_key="+authStruct.consumerKey+"&oauth_nonce="+randomString+"&oauth_signature_method=HMAC-SHA1&oauth_timestamp="+requestTime+"&oauth_token="+authStruct.accessToken+"&oauth_version=1.0&status="+std::string(curl_easy_escape(active,trendTweet.c_str(),trendTweet.length())); //why not just put in parameter?
    std::string signingKey=std::string(curl_easy_escape(active,authStruct.consumerSecret.c_str(),authStruct.consumerSecret.length()))+"&"+std::string(curl_easy_escape(active,authStruct.accessSecret.c_str(),authStruct.accessSecret.length()));
    std::string signatureBase=std::string("POST&")+curl_easy_escape(active,host_baseURL.c_str(),host_baseURL.length())+std::string("&")+curl_easy_escape(active,host_argument.c_str(),host_argument.length());
    std::string signature_=curl_easy_escape(active,base64::encode(hexStringtoASCII(hmac<SHA1>(signatureBase,signingKey).c_str())).c_str(),base64::encode(hexStringtoASCII(hmac<SHA1>(signatureBase,signingKey).c_str())).length());
    std::string oAuthHeader=std::string("Authorization: OAuth oauth_consumer_key=\"")+std::string(curl_easy_escape(active,authStruct.consumerKey.c_str(),authStruct.consumerKey.length()))+std::string("\", oauth_nonce=\""+std::string(curl_easy_escape(active,randomString.c_str(),randomString.length())))+std::string("\", oauth_signature=\"")+signature_+std::string("\", oauth_signature_method=\"HMAC-SHA1\", oauth_timestamp=\"")+requestTime+std::string("\", oauth_token=\"")+std::string(curl_easy_escape(active,authStruct.accessToken.c_str(),authStruct.accessToken.length()))+std::string("\", oauth_version=\"1.0\"");
    tokenheader=curl_slist_append(tokenheader,oAuthHeader.c_str());
    curl_easy_setopt(active,CURLOPT_POSTFIELDS,postStatusData.c_str());
    curl_easy_setopt(active,CURLOPT_URL,host_baseURL.c_str());
    curl_easy_setopt(active,CURLOPT_USERAGENT,"PostPirate v"+std::to_string(version));
    curl_easy_setopt(active,CURLOPT_WRITEFUNCTION,write_callback);
    curl_easy_setopt(active,CURLOPT_VERBOSE,verbose);
    curl_easy_setopt(active,CURLOPT_HTTPHEADER,tokenheader);
    responseData.clear();
    CURLcode response_code=curl_easy_perform(active);
    curl_easy_cleanup(active);
    return response_code;
}

std::string parseTrendTweet(std::string trendTweet) {
    std::cout<<"Parsing tweet before posting.\n";
    while(trendTweet.find("RT @")!=std::string::npos) { //Remove the author's reference
        trendTweet.erase(trendTweet.find("RT @"),trendTweet.find(":")+1);
    }
    while(trendTweet.find("@")!=std::string::npos) { //Remove any other references
        trendTweet.erase(trendTweet.find("@"),1);
    }
    return trendTweet;
}

