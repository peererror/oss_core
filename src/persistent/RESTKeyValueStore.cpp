// Library: OSS_CORE - Foundation API for SIP B2BUA
// Copyright (c) OSS Software Solutions
// Contributor: Joegen Baclor - mailto:joegen@ossapp.com
//
// Permission is hereby granted, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, execute, and to prepare
// derivative works of the Software, all subject to the
// "GNU Lesser General Public License (LGPL)".
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//


#include "OSS/Logger.h"
#include "OSS/Persistent/RESTKeyValueStore.h"
#include "Poco/Net/HTMLForm.h"
#include "Poco/Net/HTTPBasicCredentials.h"
#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/HTTPSClientSession.h"
#include "Poco/StreamCopier.h"
#include "Poco/Exception.h"


using Poco::Net::HTMLForm;
using Poco::Net::NameValueCollection;
using Poco::Net::HTTPBasicCredentials;
using Poco::Net::HTTPClientSession;
using Poco::Net::HTTPSClientSession;
using Poco::Net::HTTPResponse;
using Poco::Net::HTTPRequest; 
using Poco::StreamCopier;
using Poco::Net::HTTPMessage;
  

namespace OSS {
namespace Persistent {


//
// Used for sorting records
//
static bool compare_records (KeyValueStore::Record& first, KeyValueStore::Record& second)
{
  return first.key.compare(second.key) <= 0;
}

static void prepare_path(std::string& path)
{
  if (!OSS::string_ends_with(path, "/"))
  {
    path = path + std::string("/");
  }
}

static void get_path_vector(const std::string& path, std::vector<std::string>& pathVector)
{
  std::vector<std::string> tokens = OSS::string_tokenize(path, "/");
  
  for (std::vector<std::string>::const_iterator iter = tokens.begin(); iter != tokens.end(); iter++)
  {
    if (!iter->empty())
      pathVector.push_back(*iter);
  }
}

static std::string create_filter(const std::string& path)
{
  std::vector<std::string> tokens;
  get_path_vector(path, tokens);
  return path + std::string("*");
}

static bool printOneRecord(std::size_t filterDepth, const std::string& resourceName, std::list<KeyValueStore::Record>& records, std::list<KeyValueStore::Record>::iterator& iter, std::ostream& ostr)
{
  //
  // return false if there are no more records
  //
  if (iter == records.end())
    return false;
  
  //
  // This will hold the tokens of the current key
  //
  std::vector<std::string> keyTokens;
  get_path_vector(iter->key, keyTokens);
  
  //
  // This will hold the depth of the current item in the tree
  //
  std::size_t keyDepth = keyTokens.size() - 1;
  
  //
  // This will hold the offSet of the current key relative to the filter depth
  //
  std::size_t keyOffSet = keyDepth - filterDepth;
  
  if (keyOffSet == 0 && keyTokens[keyDepth] == resourceName)
  {
    //
    // This is an exact match.  We print it out and consume the iterator
    //
    ostr << "\"" << keyTokens[keyTokens.size() -1] << "\": " << "\"" << iter->value << "\"";
    iter++;
  }
  else if (keyOffSet > 0)
  {
    //
    // The item falls under a group of elements under the filter tree
    //
    ostr << "\"" << keyTokens[filterDepth] << "\":  {";
    
   
    while (true)
    { 
      std::string previousResource = keyTokens[filterDepth];
      if (!printOneRecord(filterDepth + 1, keyTokens[filterDepth + 1], records, iter, ostr))
        break;
      
      keyTokens.clear();
      get_path_vector(iter->key, keyTokens);
      
      if (filterDepth + 1 >= keyTokens.size())
        break;
      
      if (previousResource != keyTokens[filterDepth])
        break;
      
      ostr << ",";
    }
    
    ostr << "}";
  }
  else 
  {
    return false;
  }
  
  return iter != records.end();
}
  
RESTKeyValueStore::RESTKeyValueStore() :
  _rootDocument(REST_DEFAULT_ROOT_DOCUMENT)
{
  setHandler(boost::bind(&RESTKeyValueStore::onHandleRequest, this, _1, _2));
}
  
RESTKeyValueStore::RESTKeyValueStore(int maxQueuedConnections, int maxThreads) :
  OSS::Net::HTTPServer(maxQueuedConnections, maxThreads),
  _rootDocument(REST_DEFAULT_ROOT_DOCUMENT)
{
  setHandler(boost::bind(&RESTKeyValueStore::onHandleRequest, this, _1, _2));
}

RESTKeyValueStore::~RESTKeyValueStore()
{
  for (KVStore::const_iterator iter = _kvStore.begin(); iter != _kvStore.end(); iter++)
    delete iter->second;
}

KeyValueStore* RESTKeyValueStore::getStore(const std::string& path)
{
  std::vector<std::string> tokens = OSS::string_tokenize(path, "/");
  if(tokens.size() < 3)
    return 0;
  
  OSS::mutex_lock lock(_kvStoreMutex);
  
  KeyValueStore* pStore = 0;
  std::string document = tokens[2];
  
  if (_kvStore.find(document) == _kvStore.end())
  {
    //
    // Create a new store
    //
    std::ostringstream strm;
    
    if (!_dataDirectory.empty())
      strm << _dataDirectory << "/" << document;
    else
      strm << document;
    
    
    pStore = new KeyValueStore();
    pStore->open(strm.str());
    if (pStore->isOpen())
    {
     _kvStore.insert(std::pair<std::string, KeyValueStore*>(document, pStore));
    }
    else
    {
      delete pStore;
      pStore = 0;
    }
  }
  else
  {
    pStore = _kvStore.at(document);
  }
  
  return pStore;
}

bool RESTKeyValueStore::isAuthorized(Request& request, Response& response)
{
  if (_user.empty())
    return true;
  
  if (!request.hasCredentials())
  {
    response.requireAuthentication(request.getURI());
    response.send();
    return false;
  }
  
  HTTPBasicCredentials cred(request);
  
  bool authorized = cred.getUsername() == _user && cred.getPassword() == _password;
  
  if (!authorized)
  {
    response.setStatus(HTTPResponse::HTTP_FORBIDDEN);
    response.send();
  }
  
  return authorized;
}

void RESTKeyValueStore::onHandleRequest(Request& request, Response& response)
{ 
  if (!isAuthorized(request, response))
  {
    return;
  }
  
  if (OSS::string_starts_with(request.getURI(), _rootDocument.c_str()))
  {
    onHandleRestRequest(request, response);
    return;
  }
  
  
  if (_customHandler)
  {
    _customHandler(request, response);
    return;
  }
  
  response.setStatus(HTTPResponse::HTTP_NOT_FOUND);
  response.send();
}

int RESTKeyValueStore::restPUT(const std::string& path, const std::string& value)
{
  std::string resource = path;
  prepare_path(resource);
  
  KeyValueStore* pStore = getStore(resource);
  if (!pStore)
  {
    return HTTPResponse::HTTP_INTERNAL_SERVER_ERROR;
  }
  
  std::string data = value;
  OSS::string_replace(data, "\"", "\\\"");
  pStore->put(resource, data);
  
  return HTTPResponse::HTTP_OK;
}
    
int RESTKeyValueStore::restGET(const std::string& path, std::ostream& ostr)
{
  std::string resource = path;
  prepare_path(resource);
  
  KeyValueStore* pStore = getStore(resource);
  if (!pStore)
  {
    return HTTPResponse::HTTP_INTERNAL_SERVER_ERROR;
  }
  
  std::vector<std::string> tokens;
  get_path_vector(resource, tokens);
  std::string filter = create_filter(resource);
  
  KeyValueStore::Records records;
  pStore->getRecords(filter, records);
 
  createJSONDocument(tokens, tokens.size() - 1, records, ostr);
  
  return HTTPResponse::HTTP_OK;
}

int RESTKeyValueStore::restDELETE(const std::string& path)
{
  std::string resource = path;
  prepare_path(resource);
  
  KeyValueStore* pStore = getStore(resource);
  if (!pStore)
  {
    return HTTPResponse::HTTP_INTERNAL_SERVER_ERROR;
  }
  
  std::vector<std::string> tokens;
  get_path_vector(resource, tokens);
  std::string filter = create_filter(resource);
  
  pStore->delKeys(filter);
  
  return HTTPResponse::HTTP_OK;
}
  
void RESTKeyValueStore::onHandleRestRequest(Request& request, Response& response)
{
  if (request.getMethod() == HTTPRequest::HTTP_GET)
  {
    std::ostringstream result;
    HTTPResponse::HTTPStatus status = (HTTPResponse::HTTPStatus)restGET(request.getURI(), result);
    response.setStatus(status);
    
    if (status != HTTPResponse::HTTP_OK)
    {
      response.send();
      return;
    }
    
    response.setChunkedTransferEncoding(true);
    response.send() << result.str();
    return;
  }
  else if (request.getMethod() == HTTPRequest::HTTP_DELETE)
  {
    response.setStatus((HTTPResponse::HTTPStatus)restDELETE(request.getURI()));
    response.send();
    return;
  }
  else if (request.getMethod() == HTTPRequest::HTTP_PUT || request.getMethod() == HTTPRequest::HTTP_POST)
  {
    HTMLForm form(request, request.stream());
    if (!form.empty() && form.has("value"))
    {
      std::string value = form.get("value");
      response.setStatus((HTTPResponse::HTTPStatus)restPUT(request.getURI(), value));
      response.send();
      return;
    }
  }
  
  //
  // Send a 404 if it ever gets here
  //
  response.setStatus(HTTPResponse::HTTP_NOT_FOUND);
  response.send();
}

void RESTKeyValueStore::createJSONDocument(const std::vector<std::string>& pathVector, std::size_t depth, KeyValueStore::Records& unsorted, std::ostream& ostr)
{
  //
  // sort the records
  //
  std::list<KeyValueStore::Record> records;
  std::copy( unsorted.begin(), unsorted.end(), std::back_inserter(records));
  records.sort(compare_records);
  
 
  //
  // Loop through the records
  //
  ostr << "{";
  std::list<KeyValueStore::Record>::iterator iter = records.begin();
  while (printOneRecord(depth, pathVector[depth], records, iter, ostr))
    ostr << ",";
  ostr << "}";
}

void RESTKeyValueStore::sendRestRecordsAsJson(const std::vector<std::string>& pathVector, KeyValueStore::Records& records, Response& response)
{
  response.setChunkedTransferEncoding(true);
  response.setContentType("text/json");
  std::ostream& ostr = response.send();
  createJSONDocument(pathVector, pathVector.size() - 1, records, ostr);
}


void RESTKeyValueStore::sendRestRecordsAsValuePairs(const std::string& path, const KeyValueStore::Records& records, Response& response)
{
  //
  // sort the records
  //
  std::list<KeyValueStore::Record> sorted;
  std::copy( records.begin(), records.end(), std::back_inserter(sorted));
  sorted.sort(compare_records);
  
  
  response.setChunkedTransferEncoding(true);
  response.setContentType("text/plain");
  std::ostream& ostr = response.send();

  for (std::list<KeyValueStore::Record>::const_iterator iter = sorted.begin(); iter != sorted.end(); iter++)
  {
    ostr << iter->key << ": " << iter->value << "\r\n";
  }
}


RESTKeyValueStore::Client::Client(const std::string& host, unsigned short port) :
  _secure(false),
  _host(host),
  _port(port),
  _sessionHandle(0)
{
}

RESTKeyValueStore::Client::Client(const std::string& host, unsigned short port, bool secure) :
  _secure(secure),
  _host(host),
  _port(port),
  _sessionHandle(0)
{
}

RESTKeyValueStore::Client::~Client()
{
  delete (HTTPClientSession*)_sessionHandle;
}
    
void RESTKeyValueStore::Client::setCredentials(const std::string& user, const std::string& password)
{
  _user = user;
  _password = password;
}

bool RESTKeyValueStore::Client::execute_POST(const std::string& path, const Params& params, std::ostringstream& result, int& status)
{
  return execute(HTTPRequest::HTTP_POST, path, params, result, status);
}

bool RESTKeyValueStore::Client::execute_GET(const std::string& path, const Params& params, std::ostringstream& result, int& status)
{
  return execute(HTTPRequest::HTTP_GET, path, params, result, status);
}

bool RESTKeyValueStore::Client::execute_PUT(const std::string& path, const Params& params, std::ostringstream& result, int& status)
{
  return execute(HTTPRequest::HTTP_PUT, path, params, result,status);
}

bool RESTKeyValueStore::Client::execute_DELETE(const std::string& path, const Params& params, std::ostringstream& result, int& status)
{
  return execute(HTTPRequest::HTTP_DELETE, path, params, result, status);
}

bool RESTKeyValueStore::Client::execute(const std::string& method, const std::string& path, const Params& params, std::ostringstream& result, int& status)
{
  status = 0;
    
  try
  {
    HTTPClientSession* pSession = 0;

    if (!_sessionHandle)
    {
      if (!_secure)
        pSession = new HTTPClientSession(_host, _port);
      else
        pSession = new HTTPSClientSession(_host, _port);
        
      _sessionHandle = (OSS_HANDLE)pSession;
    }
    else
    {
      pSession = (HTTPClientSession*)_sessionHandle;
    }

         
    HTTPRequest req(method, path, HTTPMessage::HTTP_1_1);
    
    if (!_user.empty())
    {
      HTTPBasicCredentials cred(_user, _password);
      cred.authenticate(req);
    }
    
    if (!params.empty())
    {
      HTMLForm form;
      for (Params::const_iterator iter = params.begin(); iter != params.end(); iter++)
      {
        form.add(iter->first, iter->second);
      }

      // Send the request.
      form.prepareSubmit(req);
      std::ostream& ostr = pSession->sendRequest(req);
      form.write(ostr);
    }
    else
    {
      pSession->sendRequest(req);
    }
    
    // Receive the response.
	  HTTPResponse res;
    
    std::istream& rs = pSession->receiveResponse(res);
    StreamCopier::copyStream(rs, result);

    status = res.getStatus();
    
    return (status == HTTPResponse::HTTP_OK);
  }
  catch(Poco::Exception e)
  {
    OSS_LOG_ERROR("RESTKeyValueStore::Client::execute Exception: " << e.message())
    delete (HTTPClientSession*)_sessionHandle;
    _sessionHandle = 0;
    return false;
  }
}

bool RESTKeyValueStore::Client::restPUT(const std::string& path, const std::string& value, int& status)
{
  Params params;
  params["value"] = value;
  std::ostringstream result;
  bool ret = execute_PUT(path, params, result, status);
  return ret;
}
    
bool RESTKeyValueStore::Client::restGET(const std::string& path, std::ostringstream& result, int& status)
{
  Params params;
  return execute_GET(path, params, result, status);
}

bool RESTKeyValueStore::Client::restDELETE(const std::string& path, int& status)
{
  Params params;
  std::ostringstream result;
  return execute_DELETE(path, params, result, status);
}
    
} }



