// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// @file   Cru2TrackletTranslator.h
/// @brief  TRD raw data translator

#include "TRDRaw/Cru2TrackletTranslator.h"
#include "DataFormatsTRD/RawData.h"
#include "DataFormatsTRD/Tracklet64.h"
#include "DetectorsRaw/RDHUtils.h"
//#include "Headers/RAWDataHeader.h"
#include "Headers/RDHAny.h"

#include <cstring>
#include <string>
#include <vector>
#include <array>
#include <iostream>
#include <numeric>
#include <TFile.h>
#include <TH1F.h>

#define colorReset "\033[0m"
#define colorRed "\033[1;31m"
#define colorGreen "\033[1;32m"
#define colorYellow "\033[1;33m"
#define colorBlue "\033[1;34m"

namespace o2
{
namespace trd
{

const o2::header::RDHAny* find_next_rdh(const o2::header::RDHAny* rdh) {
  size_t offset = o2::raw::RDHUtils::getOffsetToNext(rdh);
  const o2::header::RDHAny* nextrdh =  (const o2::header::RDHAny*)((char*)rdh + offset);
  o2::raw::RDHUtils::printRDH(rdh);
  return nextrdh;
}

uint32_t Cru2TrackletTranslator::processHBFs()
{

    LOG(info) << colorBlue
              << "PROCESS HBF starting at " << (void*)mDataPointer
              << colorReset;
    mDataRDH = reinterpret_cast<const o2::header::RDHAny*>(mDataPointer);
    //mEncoderRDH = reinterpret_cast<o2::header::RDHAny*>(mEncoderPointer);
    auto rdh = mDataRDH;


    /* loop until RDH stop header */
    while (!o2::raw::RDHUtils::getStop(rdh)) { // carry on till the end of the event.
        LOG(info) << colorBlue
                << "--- RDH open/continue detected";
        o2::raw::RDHUtils::printRDH(rdh);



        LOG(info)<< colorReset;
        auto headerSize = o2::raw::RDHUtils::getHeaderSize(rdh);
        auto memorySize = o2::raw::RDHUtils::getMemorySize(rdh);
        auto offsetToNext = o2::raw::RDHUtils::getOffsetToNext(rdh);
        auto dataPayload = memorySize - headerSize;
        mFEEID = o2::raw::RDHUtils::getFEEID(rdh);   //TODO change this and just carry around the curreht RDH
        mCRUEndpoint=o2::raw::RDHUtils::getEndPointID(rdh);    // the upper or lower half of the currently parsed cru 0-14 or 15-29
        mCRUID=o2::raw::RDHUtils::getCRUID(rdh);
        int packetCount = o2::raw::RDHUtils::getPacketCounter(rdh);
        LOG(info) << "FEEID : " << mFEEID << " Packet: " << packetCount
                  << " sizes : header" << headerSize <<" memorysize:"<<memorySize
                  <<" offsettonext:"<< offsetToNext<<" datapayload:"<<dataPayload;
        o2::raw::RDHUtils::printRDH(rdh);
        LOG(info) << colorReset;
        /* copy CRU payload to save buffer TODO again why the copy ??*/
        // std::memcpy(mSaveBuffer + mSaveBufferDataSize, (char*)(rdh) + headerSize, drmPayload);
        // mSaveBufferDataSize += drmPayload;
        //
        // we will "simply" parse on the fly with a basic state machine.
        LOGF(info,"rdh ptr is %p\n",(void*)rdh);
        mDataPointer = (uint32_t*)((char*)rdh+headerSize);
        LOGF(info,"mDataPointer is %p  ?= %p",(void*)mDataPointer,(void*)rdh);
        printf("mDataPointer has %08x\n",mDataPointer[0]);
        mDataEndPointer = (const uint32_t*)((char*)rdh + memorySize);
        LOGF(info,"mDataEndPointer is %p\n ",(void*)mDataEndPointer);

        for (int i=0; i<0; i++) {
          printf("\n%05x  %08x", i, ((int*)mDataPointer)[i]);
        }
        if(mDataPointer[0] != 0x00000000 )
        {        //while ((void*)mDataPointer < (void*)mDataEndPointer){
          while (memorySize > 3000){ // loop to handle the case where a halfcru ends/begins mid rdh data block
              LOG(info) << colorRed << " in while loop : " <<  (void*)mDataPointer << " < " << (void*) mDataEndPointer;
              if(mDataPointer == mDataEndPointer){
                  LOG(info) << " data pointers are euqal ";
                }
              mEventCounter++;
              if (processHalfCRU()) {// at this point the entire payload is in mSaveBuffer, TODO parse this incrementally, less mem foot print.
                LOG(info) << "Completed half cru ?? ";
                break; // end of CRU
              }
              mState = CRUStateHalfChamber;
              //buildCRUPayLoad(); // the rest of the hbf for the subsequent cruhalfchamber payload.
              // TODO is this even possible if hbfs upto the stop  is for one event and each cru header is for 1 event?
              }
           }
      LOG(info) << colorGreen << " moving rdh from 0x" << (void*) rdh;
      /* move to next RDH */

        LOG(info) << colorRed << "move to next RDH" << colorReset;
        rdh = find_next_rdh(rdh);
        //rdh = (o2::header::RDHAny*)((char*)(rdh) + offsetToNext);
        //LOG(info) << " rdh is now at 0x" << (void*) rdh << " offset to next : " << offsetToNext;
        //LOG(info) << " now to loop around again hopefully ";
        o2::raw::RDHUtils::printRDH(rdh);
        printf("##############################################\n");
        LOG(info) << colorReset;

    }



    if(o2::raw::RDHUtils::getStop(rdh)) mDataPointer = (const uint32_t*)((char*)rdh + o2::raw::RDHUtils::getOffsetToNext(rdh));
    else  mDataPointer = (const uint32_t*)((char*)rdh );
    LOGF(info," at exiting processHBF after advancing to next rdh mDataPointer is %p  ?= %p",(void*)mDataPointer,(void*)mDataPointer);
    o2::raw::RDHUtils::printRDH(rdh);

    //  if (mVerbosity) {
    LOG(info) << colorBlue
      << "--- RDH close detected"
      << colorReset;
    o2::raw::RDHUtils::printRDH(rdh);
    //  }


    LOG(info) << colorBlue
      << "--- END PROCESS HBF"
      << colorReset;

      /* move to next RDH */
      // mDataPointer = (uint32_t*)((char*)(rdh) + o2::raw::RDHUtils::getOffsetToNext(rdh));

      /* otherwise return */

      return mDataEndPointer-mDataPointer;
    }

  bool Cru2TrackletTranslator::buildCRUPayLoad()
  {
    // copy data for the current half cru, and when we eventually get to the end of the payload return 1
    // to say we are done.
    int cruid=0;
    int additionalBytes=-1;
    int crudatasize=-1;
    LOG(info) << colorRed
        << "--- Build CRU Payload, added " << additionalBytes << " bytes to CRU "
        << cruid << " with new size " << crudatasize
        << colorReset;
    return true;
  }


  bool Cru2TrackletTranslator::processHalfCRU()
  {
    /* process a FeeID/halfcru, 15 links */
    LOG(info) << colorGreen << "--- PROCESS HalfCRU FeeID:" << mFEEID
        << colorReset;
    mCurrentLinkDataPosition=0;
    if(mState == CRUStateHalfCRUHeader){
        // well then read the halfcruheader.
        memcpy(&mCurrentHalfCRUHeader,(void*)(mDataPointer),sizeof(mCurrentHalfCRUHeader));
        //mEncoderRDH = reinterpret_cast<o2::header::RDHAny*>(mEncoderPointer);)
        mCurrentLink=0;
        o2::trd::getlinkdatasizes(mCurrentHalfCRUHeader,mCurrentHalfCRULinkLengths);
        o2::trd::getlinkerrorflags(mCurrentHalfCRUHeader,mCurrentHalfCRULinkErrorFlags);
        mTotalHalfCRUDataLength = std::accumulate(mCurrentHalfCRULinkLengths.begin(),
                                                  mCurrentHalfCRULinkLengths.end(),
                                                  decltype(mCurrentHalfCRULinkLengths)::value_type(0));
        // we will always have at least a length of 1 fully padded for each link.
        LOG(info) << colorGreen << "Found  a HalfCRUHeader : ";
        LOG(info) << mCurrentHalfCRUHeader
                  << colorReset;
        mState=CRUStateHalfChamber; // we expect a halfchamber header now
        //TODO maybe change name to something more generic, this is will have to change to handle other data types config/adcdata.
    }
    printf("about to while  %p != %p && %d != %d\n",(void*)mDataPointer,(void*)mDataEndPointer, mCurrentLinkDataPosition, mTotalHalfCRUDataLength*16);

//     while(mDataPointer!=mDataEndPointer && mCurrentLinkDataPosition != mTotalHalfCRUDataLength*16)
//     { // while we are stil in the rdh block and with in the current link
//         LOG(info) << "in while loop with state of :" << mState;
//         if(mState==CRUStateHalfChamber){
//
//
//           if(mDataPointer[0] != 0xeeeeeeee ){
// //
//            // read in the halfchamber header.
//            memcpy(&mCurrentHalfChamber,(void*)(mDataPointer),sizeof(mCurrentHalfChamber));
//
//            LOG(info) << colorGreen << "Found  a HalfChamberHeader : ";
//            LOG(info) << mCurrentHalfChamber
//                      << colorReset;
//            mState=CRUStateDigitMCMHeader; // we expect a MCM header now
//
//
//             LOGF(info,"mDigitHCHeader is at %p had value 0x%08x",(void*)mDataPointer,mDataPointer[0]);
//             mDigitHCHeader=(DigitHCHeader*)mDataPointer;
//             mDataPointer+=16;//sizeof(mDigitHCHeader)/4;
//
//            mState=CRUStateDigitMCMHeader; //now we expect a TrackletMCMHeader or some padding.
//         }
//        }
//
//         if(mState==CRUStateDigitMCMHeader){
//
//            memcpy(&mCurrentDigitMCMHeader,(void*)(mDataPointer),sizeof(mCurrentDigitMCMHeader));
//
//            LOG(info) << colorGreen << "Found  a MCMHeader : ";
//            LOG(info) << mCurrentDigitMCMHeader
//                     << colorReset;
//            mState=CRUStateDigitMCMData; // we expect a MCM header now
//
//              LOGF(info,"mDigitMCMHeader is at %p had value 0x%08x",(void*)mDataPointer,mDataPointer[0]);
//                 if(debugparsing){
//          //           LOG(info) << " state is : " << mState << " about to read TrackletMCMHeader";
//                 }
//             //read the header OR padding of 0xeeee;
//             if(mDataPointer[0] != 0xeeeeeeee){
//                 //we actually have an header word.
//                 mDigitHCHeader = (DigitHCHeader*)mDataPointer;
//               LOG(info) << colorBlue << "state mcmheader and word : 0x" << std::hex << mDataPointer[0];
//                 mDataPointer++;
//                 mCurrentLinkDataPosition++;
//                 if(debugparsing){
//            //l       printTrackletMCMHeader(*mTrackletHCHeader);
//                 }
//                 mState=CRUStateDigitMCMData;
//             }
//             else { // this is the case of a first padding word for a "noncomplete" tracklet i.e. not all 3 tracklets.
//             //        LOG(info) << "C";
//                 mState=CRUStatePadding;
//                 mDataPointer++;
//                 mCurrentLinkDataPosition++;
//                 //TRDStatCounters.LinkPadWordCounts[mHCID]++; // keep track off all the padding words.
//                 if(debugparsing){
//            //       printTrackletMCMHeader(*mTrackletHCHeader);
//                 }
//             }
//         }
//         if(mState==CRUStatePadding){
//             LOGF(info,"Padding is at %p had value 0x%08x",(void*)mDataPointer,mDataPointer[0]);
//       //      LOG(info) << colorBlue << "state padding and word : 0x" << std::hex << mDataPointer[0];
//             if(mDataPointer[0] == 0xeeeeeeee){
//                 //another pointer with padding.
//                 mDataPointer++;
//                 mCurrentLinkDataPosition++;
//                 TRDStatCounters.LinkPadWordCounts[mHCID]++; // keep track off all the padding words.
//                 if(mDataPointer[0] & 0x1 ){
//                     //mcmheader
//             //        LOG(info) << "changing state from padding to mcmheader as next datais 0x" << std::hex << mDataPointer[0];
//                     mState=CRUStateDigitMCMHeader;
//                 }
//                 else
//                     if(mDataPointer[0] != 0xeeeeeeee ){
//           //        LOG(info) << "changing statefrom padding to mcmdata as next datais 0x" << std::hex << mDataPointer[0];
//                     mState=CRUStateDigitMCMData;
//                 }
//             }
//             else{
//                 LOG(info) << colorRed << "some went wrong we are in state padding, but not a pad word. 0x" << (void*)mDataPointer<< colorReset;
//             }
//         }
//         if(mState==CRUStateDigitMCMData){
//             LOGF(info,"mDigitMCMData is at %p had value 0x%08x",(void*)mDataPointer,mDataPointer[0]);
//             //tracklet data;
           // build tracklet.
           //for the case of on flp build a vector of tracklets, then pack them into a data stream with a header.
           //for dpl build a vector and connect it with a triggerrecord.
        //    mDigitMCMData = (DigitMCMData*)mDataPointer;
        //    mDataPointer++;
        //    mCurrentLinkDataPosition++;
        //    if(mDataPointer[0]==0xeeeeeeee){
        //        mState=CRUStatePadding;
        //      //  LOG(info) <<"changing to padding from mcmdata" ;
        //    }
        //    else {
        // //            LOG(info) << "J";
        //         if(mDataPointer[0]&0x1){
        // //            LOG(info) << "K";
        //             mState=CRUStateDigitMCMHeader; // we have more tracklet data;
        //          //   LOG(info) <<"changing from MCMData to MCMHeader" ;
        //         }
        //         else{
        //             mState=CRUStateDigitMCMData;
        //        //     LOG(info) <<"continuing with mcmdata" ;
        //         }
        //    }
          // Tracklet64 trackletsetQ0(o2::trd::getTrackletQ0());
      //  }
        //accounting ....
       // mCurrentLinkDataPosition256++;
       // mCurrentHalfCRUDataPosition256++;
       // mTotalHalfCRUDataLength++;
        LOG(info) << colorRed << mDataPointer <<":" << mDataEndPointer  << " &&  " <<  mCurrentLinkDataPosition  << " != " <<  mTotalHalfCRUDataLength*16 << colorReset;

    //}
    //end of data so
    /* init decoder */
    mDataNextWord = 1;
    mError = false;
    mFatal = false;

    /* check TRD Data Header */

    LOG(info) << colorRed << std::hex << *mDataPointer << " [ERROR] trying to recover CRU half chamber" << colorRed << colorReset;
    LOG(info) << colorRed << mDataPointer <<":" << mDataEndPointer  << " &&  " <<  mCurrentLinkDataPosition  << " != " <<  mTotalHalfCRUDataLength*16 << colorReset;
    LOG(info) << colorBlue << "state  == " << mState;

    LOG(info) << colorGreen
        << "--- END PROCESS HalfCRU"
        << colorReset;

    return true;
}


// bool Cru2TrackletTranslator::processHalfCRU()
// {
//     /* process a FeeID/halfcru, 15 links */
//     LOG(info) << colorGreen << "--- PROCESS HalfCRU FeeID:" << mFEEID
//         << colorReset;
//     mCurrentLinkDataPosition=0;
//     if(mState == CRUStateHalfCRUHeader){
//         // well then read the halfcruheader.
//         memcpy(&mCurrentHalfCRUHeader,(void*)(mDataPointer),sizeof(mCurrentHalfCRUHeader));
//         //mEncoderRDH = reinterpret_cast<o2::header::RDHAny*>(mEncoderPointer);)
//         mCurrentLink=0;
//         o2::trd::getlinkdatasizes(mCurrentHalfCRUHeader,mCurrentHalfCRULinkLengths);
//         o2::trd::getlinkerrorflags(mCurrentHalfCRUHeader,mCurrentHalfCRULinkErrorFlags);
//         mTotalHalfCRUDataLength = std::accumulate(mCurrentHalfCRULinkLengths.begin(),
//                                                   mCurrentHalfCRULinkLengths.end(),
//                                                   decltype(mCurrentHalfCRULinkLengths)::value_type(0));
//         // we will always have at least a length of 1 fully padded for each link.
//         LOG(info) << colorGreen << "Found  a HalfCRUHeader : ";
//         LOG(info) << mCurrentHalfCRUHeader
//                   << colorReset;
//         mState=CRUStateHalfChamber; // we expect a halfchamber header now
//         //TODO maybe change name to something more generic, this is will have to change to handle other data types config/adcdata.
//     }
//     printf("about to while  %p != %p && %d != %d\n",(void*)mDataPointer,(void*)mDataEndPointer, mCurrentLinkDataPosition, mTotalHalfCRUDataLength*16);
//     while(mDataPointer!=mDataEndPointer && mCurrentLinkDataPosition != mTotalHalfCRUDataLength*16)
//     { // while we are stil in the rdh block and with in the current link
//         LOG(info) << "in while loop with state of :" << mState;
//         if(mState==CRUStateHalfChamber){
//            // read in the halfchamber header.
//             LOGF(info,"mTrackletHCHeader is at %p had value 0x%08x",(void*)mDataPointer,mDataPointer[0]);
//             mTrackletHCHeader=(TrackletHCHeader*)mDataPointer;
//             mDataPointer+=16;//sizeof(mTrackletHCHeader)/4;
//           //  mHCID=mTrackletHCHeader->HCID;
//        //     LOGF(info,"mDataPointer after advancing past TrackletHCHeader is at %p has value 0x%08x",(void*)mDataPointer,mDataPointer[0]);
//             //if(debugparsing){
//            //     printHalfChamber(*mTrackletHCHeader);
//            // }
//            mState=CRUStateTrackletMCMHeader; //now we expect a TrackletMCMHeader or some padding.
//         }
//         if(mState==CRUStateTrackletMCMHeader){
//             LOGF(info,"mTrackletMCMHeader is at %p had value 0x%08x",(void*)mDataPointer,mDataPointer[0]);
//                 if(debugparsing){
//          //           LOG(info) << " state is : " << mState << " about to read TrackletMCMHeader";
//                 }
//             //read the header OR padding of 0xeeee;
//             if(mDataPointer[0] != 0xeeeeeeee){
//                 //we actually have an header word.
//                 mTrackletHCHeader = (TrackletHCHeader*)mDataPointer;
//        ///     LOG(info) << colorBlue << "state mcmheader and word : 0x" << std::hex << mDataPointer[0];
//                 mDataPointer++;
//            mCurrentLinkDataPosition++;
//                 if(debugparsing){
//            //l       printTrackletMCMHeader(*mTrackletHCHeader);
//                 }
//                 mState=CRUStateTrackletMCMData;
//             }
//             else { // this is the case of a first padding word for a "noncomplete" tracklet i.e. not all 3 tracklets.
//             //        LOG(info) << "C";
//                 mState=CRUStatePadding;
//                 mDataPointer++;
//            mCurrentLinkDataPosition++;
//                 TRDStatCounters.LinkPadWordCounts[mHCID]++; // keep track off all the padding words.
//                 if(debugparsing){
//            //       printTrackletMCMHeader(*mTrackletHCHeader);
//                 }
//             }
//         }
//         if(mState==CRUStatePadding){
//             LOGF(info,"Padding is at %p had value 0x%08x",(void*)mDataPointer,mDataPointer[0]);
//       //      LOG(info) << colorBlue << "state padding and word : 0x" << std::hex << mDataPointer[0];
//             if(mDataPointer[0] == 0xeeeeeeee){
//                 //another pointer with padding.
//                 mDataPointer++;
//            mCurrentLinkDataPosition++;
//                 TRDStatCounters.LinkPadWordCounts[mHCID]++; // keep track off all the padding words.
//                 if(mDataPointer[0] & 0x1 ){
//                     //mcmheader
//             //        LOG(info) << "changing state from padding to mcmheader as next datais 0x" << std::hex << mDataPointer[0];
//                     mState=CRUStateTrackletMCMHeader;
//                 }
//                 else
//                     if(mDataPointer[0] != 0xeeeeeeee ){
//           //        LOG(info) << "changing statefrom padding to mcmdata as next datais 0x" << std::hex << mDataPointer[0];
//                     mState=CRUStateTrackletMCMData;
//                 }
//             }
//             else{
//                 LOG(info) << colorRed << "some went wrong we are in state padding, but not a pad word. 0x" << (void*)mDataPointer<< colorReset;
//             }
//         }
//         if(mState==CRUStateTrackletMCMData){
//             LOGF(info,"mTrackletMCMData is at %p had value 0x%08x",(void*)mDataPointer,mDataPointer[0]);
//             //tracklet data;
//            // build tracklet.
//            //for the case of on flp build a vector of tracklets, then pack them into a data stream with a header.
//            //for dpl build a vector and connect it with a triggerrecord.
//            mTrackletMCMData = (TrackletMCMData*)mDataPointer;
//            mDataPointer++;
//            mCurrentLinkDataPosition++;
//            if(mDataPointer[0]==0xeeeeeeee){
//                mState=CRUStatePadding;
//              //  LOG(info) <<"changing to padding from mcmdata" ;
//            }
//            else {
//         //            LOG(info) << "J";
//                 if(mDataPointer[0]&0x1){
//         //            LOG(info) << "K";
//                     mState=CRUStateTrackletMCMHeader; // we have more tracklet data;
//                  //   LOG(info) <<"changing from MCMData to MCMHeader" ;
//                 }
//                 else{
//                     mState=CRUStateTrackletMCMData;
//                //     LOG(info) <<"continuing with mcmdata" ;
//                 }
//            }
//           // Tracklet64 trackletsetQ0(o2::trd::getTrackletQ0());
//         }
//         //accounting ....
//        // mCurrentLinkDataPosition256++;
//        // mCurrentHalfCRUDataPosition256++;
//        // mTotalHalfCRUDataLength++;
//         LOG(info) << colorRed << mDataPointer <<":" << mDataEndPointer  << " &&  " <<  mCurrentLinkDataPosition  << " != " <<  mTotalHalfCRUDataLength*16 << colorReset;
//
//     }
    //end of data so
    /* init decoder */
//     mDataNextWord = 1;
//     mError = false;
//     mFatal = false;
//
//     /* check TRD Data Header */
//
//     LOG(info) << colorRed << std::hex << *mDataPointer << " [ERROR] trying to recover CRU half chamber" << colorRed << colorReset;
//     LOG(info) << colorRed << mDataPointer <<":" << mDataEndPointer  << " &&  " <<  mCurrentLinkDataPosition  << " != " <<  mTotalHalfCRUDataLength*16 << colorReset;
//     LOG(info) << colorBlue << "state  == " << mState;
//
//     LOG(info) << colorGreen
//         << "--- END PROCESS HalfCRU"
//         << colorReset;
//
//     return true;
// }

bool Cru2TrackletTranslator::processCRULink()
{
    /* process a CRU Link 15 per half cru */
    //  checkFeeID(); // check the link we are working with corresponds with the FeeID we have in the current rdh.
    //  uint32_t slotId = GET_TRMDATAHEADER_SLOTID(*mDataPointer);
    return false;
}

bool Cru2TrackletTranslator::checkerCheck()
{
    /* checker check */


    LOG(info) << colorBlue
        << "--- CHECK EVENT"
        << colorReset;

    return false;
}

void Cru2TrackletTranslator::checkerCheckRDH()
{
    /* check orbit */
    //   LOGF(info," --- Checking HalfCRU/RDH orbit: %08x/%08x \n", orbit, getOrbit(mDatardh));
    //  if (orbit != mDatardh->orbit) {
    //      LOGF(info," HalfCRU/RDH orbit mismatch: %08x/%08x \n", orbit, getOrbit(mDatardh));
    //  }

    /* check FEE id */
    //   LOGF(info, " --- Checking CRU/RDH FEE id: %d/%d \n", mcruFeeID, getFEEID(mDatardh) & 0xffff);
    //  if (mcruFeeID != (mDatardh->feeId & 0xFF)) {
    //      LOGF(info, " HalfCRU/RDH FEE id mismatch: %d/%d \n", mcruFeeID, getFEEID(mDatardh) & 0xffff);
    //  }
}

void Cru2TrackletTranslator::resetCounters()
{
    mEventCounter = 0;
    mFatalCounter = 0;
    mErrorCounter = 0;
}

void Cru2TrackletTranslator::checkSummary()
{
    char chname[2] = {'a', 'b'};

    LOG(info) << colorBlue
        << "--- SUMMARY COUNTERS: " << mEventCounter << " events "
        << " | " << mFatalCounter << " decode fatals "
        << " | " << mErrorCounter << " decode errors "
        << colorReset;
    std::cout << "check summary a log message should be avove this" << std::endl;
}


} // namespace trd
} // namespace o2
