#include <asio.hpp>
#include <boost/function.hpp>
#include <iostream>

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>

#include "FlowDtlsSocketContext.hxx"
#include "FlowManagerSubsystem.hxx"

using namespace flowmanager;
using namespace resip;
using namespace dtls;
using namespace std;

#define RESIPROCATE_SUBSYSTEM FlowManagerSubsystem::FLOWMANAGER

FlowDtlsSocketContext::FlowDtlsSocketContext(Flow& flow, const asio::ip::address& address, unsigned short port) 
   : mFlow(flow), mAddress(address), mPort(port), mSrtpInitialized(false)
{
}

FlowDtlsSocketContext::~FlowDtlsSocketContext() 
{
   if(mSrtpInitialized)
   {
      // Free the master key memory allocated in DtlsSocket::createSrtpSessionPolicies
      delete mSRTPPolicyIn.key;
      delete mSRTPPolicyOut.key;
   }
}

void 
FlowDtlsSocketContext::write(const unsigned char* data, unsigned int len)
{
   InfoLog(<< "Dtls write to " << mAddress << ":" << mPort << " called.  ComponentId=" << mFlow.getComponentId());
   mFlow.rawSendTo(mAddress, mPort, (const char*)data, len);
}

void 
FlowDtlsSocketContext::handshakeCompleted()
{
   InfoLog(<< "Flow Dtls Handshake Completed!  ComponentId=" << mFlow.getComponentId());

   char fprint[100];
   SRTP_PROTECTION_PROFILE *srtp_profile;
   int r;

   if(mSocket->getRemoteFingerprint(fprint))
   {
      Data remoteSDPFingerprint = mFlow.getRemoteSDPFingerprint();
      if(!remoteSDPFingerprint.empty())
      {
         if(!mSocket->checkFingerprint(remoteSDPFingerprint.c_str(), remoteSDPFingerprint.size()))
         {
            InfoLog(<< "Remote fingerprint = " << fprint << " is not valid!  ComponentId=" << mFlow.getComponentId());
            return;
         }
         else
         {
            InfoLog(<< "Remote fingerprint = " << fprint << " is valid!  ComponentId=" << mFlow.getComponentId());
         }
      }
      else
      {
         InfoLog(<< "Remote fingerprint = " << fprint << "  ComponentId=" << mFlow.getComponentId());
      }
   } 
   else
   {
      InfoLog(<< "Remote fingerprint cannot be obtained from Dtls handshake.  ComponentId=" << mFlow.getComponentId());
      return; 
   }

   srtp_profile=mSocket->getSrtpProfile();

   if(srtp_profile)
   {
      InfoLog(<< "SRTP Extension negotiated profile=" << srtp_profile->name << "  ComponentId=" << mFlow.getComponentId());
   }

   mSocket->createSrtpSessionPolicies(mSRTPPolicyOut, mSRTPPolicyIn);

   r=srtp_create(&mSRTPSessionIn, &mSRTPPolicyIn);   
   assert(r==0);
   r=srtp_create(&mSRTPSessionOut, &mSRTPPolicyOut);
   assert(r==0);
   mSrtpInitialized = true;
}
 
void 
FlowDtlsSocketContext::handshakeFailed(const char *err)
{
   ErrLog(<< "Flow Dtls Handshake failed!  ComponentId=" << mFlow.getComponentId());
}

void FlowDtlsSocketContext::fingerprintMismatch()
{
   // Ensure Srtp is not initalized, so the will not process media packets from this endpoint
   if(mSrtpInitialized)
   {
      // Free the master key memory allocated in DtlsSocket::createSrtpSessionPolicies
      delete mSRTPPolicyIn.key;
      delete mSRTPPolicyOut.key;
   }
   mSrtpInitialized = false;
}

err_status_t 
FlowDtlsSocketContext::srtpProtect(void* data, int* size, bool rtcp)
{
   err_status_t status = err_status_no_ctx;
   if(mSrtpInitialized)
   {
      if(rtcp)
      {
         status = srtp_protect_rtcp(mSRTPSessionOut, data, size);  
      }
      else
      {
         status = srtp_protect(mSRTPSessionOut, data, size);  
      }
   }
   return status;
}

err_status_t 
FlowDtlsSocketContext::srtpUnprotect(void* data, int* size, bool rtcp)
{
   err_status_t status = err_status_no_ctx;
   if(mSrtpInitialized)
   {
      if(rtcp)
      {
         status = srtp_unprotect_rtcp(mSRTPSessionIn, data, size);  
      }
      else
      {
         status = srtp_unprotect(mSRTPSessionIn, data, size);  
      }
   }
   return status;
}


/* ====================================================================

 Original contribution Copyright (C) 2008 Plantronics, Inc.
 Provided under the terms of the Vovida Software License, Version 2.0.

 The Vovida Software License, Version 2.0 
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:
 
 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
 
 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the
    distribution. 
 
 THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 DAMAGE.

 ==================================================================== */