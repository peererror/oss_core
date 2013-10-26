// OSS Software Solutions Application Programmer Interface
// Author: Joegen E. Baclor - mailto:joegen@ossapp.com
//
// Copyright (c) OSS Software Solutions
//
// Permission is hereby granted, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, execute, and to prepare
// derivative works of the Software, all subject to the
// GNU Lesser General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version..
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//


#ifndef OSS_EXEC_MANAGED_DAEMON_H_INCLUDED
#define	OSS_EXEC_MANAGED_DAEMON_H_INCLUDED


#include "OSS/Exec/Process.h"
#include "OSS/IPCQueue.h"

namespace OSS {
namespace Exec {

class ManagedDaemon : boost::noncopyable
{
public:
  ManagedDaemon(const std::string& executablePath, 
                const std::string& alias,
                const std::string& runDirectory,
                const std::string& startupScript,
                const std::string& shutdownScript,
                const std::string& pidFile,
                int maxRestart);
  /// Constructor
  /// Parameters:
  ///  - executablePath : The absolute path for the executable
  ///  - alias : The name to be used for the soft link as well as the ipc queue prefix
  ///  - runDirectory : The place where pid file, soft link and IPC queue file will be stored.
  ///    The application MUST have write access to the runDirectory
  ///  - startupScript :  The command or script to be executed to run the daemon
  ///  - shutdownScript:  The command or script to be executed to stop the daemon
  ///  - pidFile: The path to the pid file generated by the daemon
  ///  - maxRestart:  Maximum number of consecutive restart before backing off
  
  ~ManagedDaemon();
  /// Destructor
  
  bool isAlive() const;
  /// Description: Checks if process is alive
  /// Returns:
  ///  true : Process is running
  ///  false : Process is dead
  
  int getPid() const;
  /// Description: Get the PID of the running process
  /// Returns:
  ///   - None Zero positive integer if the process is running
  ///   - (-1) if process is not running
  
  bool readMessage(std::string& message, bool blocking = true);
  /// Description: Read an IPC message from the daemon message queue
  /// Parameters:
  ///   - message : Bytes received
  ///   - blocking : If set to true, the function will block until the next 
  ///     IPC message is received.  If false, the function will return right away.
  ///     The default value is blocking.
  /// Returns:
  ///   - false : If there are no message in the queue
  ///   - true : If an IPC message has been read
  
  bool stop();
  /// Description: Stops the running process.
  /// Returns:
  ///   - true : Successful operation
  ///   - false : Unable to stop the running process
  
  bool start();
  /// Description: Starts the running process.
  /// Returns:
  ///   - true : Successful operation
  ///   - false : Unable to start the process 
  
  bool restart();
  /// Description:  Restarts a running process
  /// Returns:
  ///   - true : Successful operation
  ///   - false : Unable to start the process 
  /// Note: If process is not running, operation is similar to start()
  
  const std::string& getAlias() const;
  /// Description: Get the alias of the process
  /// Returns: 
  ///   - alias : The name to be used for the soft link as well as the IPC queue prefix
  
  const std::string& getPath() const;
  /// Description: Get the path to the executable
  /// Returns:
  ///   - path : Absolute path of the executable file
  
  const std::string& getRunDirectory() const;
  /// Description: Get the path of the run directory
  /// Returns:
  ///   - runDirectory : Absolute path of the run directory
  
  virtual Process::Action onDeadProcess(int consecutiveCount);
  /// Description: Handler called when the daemon dies unexpectedly.
  /// Parameters:
  ///   - consecutiveCount : Number of consecutive times the process died and restarted
  /// Returns: 
  ///  - Process::ProcessRestart (default) : Restart the process
  ///  - Process::ProcessBackoff : Do not restart the process right away until the back off time has lapsed
  ///  - Process::ProcessUnmonitor : Stop monitoring the process
 
  
protected:
  std::string _path; /// The path to the executable
  std::string _alias; /// The name to be used for the soft link as well as the IPC queue prefix
  std::string _runDirectory;  /// The run directory where PID file IPC queue files are located
  int _maxRestart; /// Maximum number of consecutive restart before backing off
  Process* _pProcess; /// The process object
};

//
// Inlines
//
  
inline const std::string& ManagedDaemon::getAlias() const
{
  return _alias;
}

inline const std::string& ManagedDaemon::getPath() const
{
  return _path;
}

inline const std::string& ManagedDaemon::getRunDirectory() const
{
  return _runDirectory;
}

} } // OSS::Exec

#endif // OSS_EXEC_MANAGED_DAEMON_H_INCLUDED
