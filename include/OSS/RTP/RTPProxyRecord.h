/*
 * Copyright (C) 2012  OSS Software Solutions
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with main.c; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */


#ifndef OSS_RTPPROXYRECORD_H_INCLUDED
#define	OSS_RTPPROXYRECORD_H_INCLUDED

#include "OSS/build.h"
#if ENABLE_FEATURE_RTP

#include "OSS/Persistent/RedisClient.h"
#include <boost/filesystem.hpp>

namespace OSS {
namespace RTP {

struct SBCMediaRecord
{
    std::string identifier;
    std::string localEndPointLeg1;
    std::string localEndPointLeg2;
    std::string senderEndPointLeg1;
    std::string senderEndPointLeg2;
    std::string lastSenderEndPointLeg1;
    std::string lastSenderEndPointLeg2;
    bool adjustSenderFromPacketSource;
    bool leg1Reset;
    bool leg2Reset;
    bool isStarted;
    bool isInactive;
    bool isLeg1XOREncrypted;
    bool isLeg2XOREncrypted;
};

struct SBCMediaTuple
{
  SBCMediaRecord data;
  SBCMediaRecord control;
};

struct RTPProxyRecord
{
  RTPProxyRecord();
#if ENABLE_FEATURE_REDIS
  bool writeToRedis(Persistent::RedisBroadcastClient& client, const std::string& key) const;
  bool writeToRedis(Persistent::RedisBroadcastClient& client, const boost::filesystem::path& key) const;
  bool readFromRedis(Persistent::RedisBroadcastClient& client, const boost::filesystem::path& key);
  bool readFromRedis(Persistent::RedisBroadcastClient& client, const std::string& key);
#endif
  
  std::string identifier;
  std::string logId;
  std::string leg1Identifier;
  std::string leg2Identifier;
  std::string leg1OriginAddress;
  std::string leg2OriginAddress;
  std::string lastSDPInAck;
  bool isExpectingInitialAnswer;
  bool hasOfferedAudioProxy;
  bool hasOfferedVideoProxy;
  bool hasOfferedFaxProxy;
  bool isAudioProxyNegotiated;
  bool isVideoProxyNegotiated;
  bool isFaxProxyNegotiated;
  bool verbose;
  int state;
  int lastOfferIndex;
  OSS::UInt64 timestamp;
  SBCMediaTuple audio;
  SBCMediaTuple video;
  SBCMediaTuple fax;
};

//
// Inlines
//
#if ENABLE_FEATURE_REDIS
inline bool RTPProxyRecord::writeToRedis(Persistent::RedisBroadcastClient& client, const boost::filesystem::path& key) const
{
  return writeToRedis(client, OSS::boost_file_name(key));
}

inline bool RTPProxyRecord::readFromRedis(Persistent::RedisBroadcastClient& client, const boost::filesystem::path& key)
{
  return readFromRedis(client, OSS::boost_file_name(key));
}
#endif

} } // OSS::RTP

#endif // ENABLE_FEATURE_RTP

#endif	// OSS_RTPPROXYRECORD_H_INCLUDED

