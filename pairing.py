#!/usr/bin/python
import os
import sys
import re
import signal
import pexpect
import argparse
from functools import wraps

APP_PROMPT = 'weave-device-mgr '

os.chdir(os.path.dirname(os.path.abspath(__file__)))
def functTimeout(seconds=600, error_message="%ds timeout with signal %s on frame %s"):
    """
    A decorator which will be used to wrap function calls to make sure
    they don't hang
    """
    def decorate(func):
        """ The decorator funct """
        def _handle_timeout(signum, frame):
            """ decorator action """
            print >> sys.stderr, error_message % (seconds, signum, frame)
            os.killpg(os.getpgid(os.getpid()), signal.SIGKILL)
        def wrapper(*args, **kwargs):
            """ decorator wrapper """
            signal.signal(signal.SIGALRM, _handle_timeout)
            signal.alarm(seconds)
            try:
                res = func(*args, **kwargs)
            finally:
                signal.alarm(0)
            return res
        return wraps(func)(wrapper)
    return decorate

@functTimeout()
class WeaveDeviceManager(object):
    """
    This class is the python interface to the weave device manager
    """
    @functTimeout(120)
    def __init__(self,
                 ipv6Addr=None,
                 logfd=sys.stdout):
        """
        Initialization
        """
        app = os.path.join(os.environ['WDMRoot'], '', 'weave-device-mgr')
        print app
        self.app = pexpect.spawn(app, logfile=logfd)
        self.app.delaybeforesend = 1
        self.app.expect([APP_PROMPT, pexpect.TIMEOUT], timeout=10)
    def close(self):
        """
        Exit out of the app (if it exists) and close all opened peripherals
        """
        if hasattr(self, 'app'):
            self.chat('close', timeout=30)
            self.app.sendline('exit')
    def __del__(self):
        """
        The destructor
        """
        if self.app.isalive():
            try:
                self.app.terminate(force=True)
                self.app.kill(9)
            except OSError, EOF:
                pass
    def chat(self, cmd, timeout=7, interrupt_on_timeout=False):
        """
        Send a command to weave device manager and return the results
        """
        self.app.delaybeforesend = 1
        self.app.setecho(False)
        self.app.send(cmd+'\n')
        index = self.app.expect(
            [APP_PROMPT, pexpect.TIMEOUT],
            timeout=timeout
        )
        buf = self.app.before
        # print buf
        dummy, rest = buf.split('\r\n', 1)      # strip echoed command\
        if index == 0:
            return rest
        elif interrupt_on_timeout:
            print "Timeout, sending Ctrl-C"
            self.app.sendintr()
            self.app.expect(APP_PROMPT, timeout=100)
            return rest
        else:
            raise pexpect.TIMEOUT(
                "Timeout waiting for response to command '%s'" % cmd
            )

def WaitForInput(PromptsDisabled=False):
    if PromptsDisabled is False:
        ready = raw_input("Press y/Y to continue: ")
        if ready is not 'y' and ready is not 'Y':
            print "Exiting\n"
            exit(0)

def ble_scan_connect(wdm, ble_identifier):
    timeout_threshold = 2
    timer_counter = 0
    connected = False

    while timer_counter <= timeout_threshold:
        timer_counter += 1
        """
        ble-scan-connect command
        """
        try:
            output = wdm.chat('ble-scan-connect -t 120  ' + ble_identifier, timeout=60)
        except:
            raise RuntimeError("Process failed at ble-scan-connect")
        if 'connect success' not in output:
            continue
        else:
            connected = True
            break
    if not connected:
        raise RuntimeError("Unable to find first device during ble-scan. Timed out after 2 scans.")

def connect(wdm, case=None, pase=None):

    """
    connect command
    """
    if pase is not None:
        command = "connect -b -p " + pase
    elif case is not None:
        command = "connect -b -a " + case
    else:
        command = "connect -b"

    try:
        output = wdm.chat(command, timeout=15)
    except:
        raise RuntimeError("Process failed at connect")
    if 'Connected to device' not in output:
        raise RuntimeError("Unable to connect.")

def close(wdm):
    """
    close command
    """
    try:
        output = wdm.chat('close', timeout=60)
    except:
        raise RuntimeError("Process failed at close")
    if 'Closing endpoints' not in output:
        raise RuntimeError("Unable to close on first device.")

def enable_network(wdm, network_id):
    """
    enable network command
    """
    try:
        output = wdm.chat('enable-network ' + network_id, timeout=60)
    except:
        raise RuntimeError("Process failed at enable-network")
    if 'Enable network complete' not in output:
        raise RuntimeError("Unable to enable network.")

def test_network(wdm, network_id):
    """
    test network command
    """
    try:
        output = wdm.chat('test-network ' + network_id, timeout=60)
    except:
        raise RuntimeError("Process failed at test-network")
    if 'Network test complete' not in output:
        raise RuntimeError("Unable to test network.")

def register_service(wdm, command):
    """
    register-service command
    """
    try:
        output = wdm.chat('' + command, timeout=60)
    except:
        raise RuntimeError("Process failed at register-service")
    if 'Register service complete' not in output:
        raise RuntimeError("Unable to register-service.")

def main():
    if 'WDMRoot' not in os.environ:
        raise RuntimeError(
            'Run "export WDMRoot=<base-path-to-folder-containing-weave-device-mgr>"'
        )

    parser = argparse.ArgumentParser(description="Pairing Helper Script.")

    parser.add_argument('--pairingcode_firstdevice', help="Pairing Code of First Device", default='')
    parser.add_argument('--pairingcode_seconddevice', help="Pairing Code of Second Device", default='NESTUS')

    parser.add_argument('--ble_firstdevice', help="BLE Identifier for First Device", default='')
    parser.add_argument('--ble_seconddevice', help="BLE Identifier for Second Device", default='')

    parser.add_argument('--wifi_name', help="Wifi SSID", default='')
    parser.add_argument('--wifi_security_type', help="Wifi Security Type", default='wpa2')
    parser.add_argument('--wifi_password', help="Wifi Password", default='')

    parser.add_argument('--register_firstdevice', help="Path to text file that holds the register service command for first device", default='')
    parser.add_argument('--register_seconddevice', help="Path to text file that holds the register service command for second device", default='')

    parser.add_argument('--second_device_only', help="Go through second device pairing only", default='False')
    parser.add_argument('--first_device_info', help="Get Information from first device", default='False')

    parser.add_argument('--thread_networkname', help="Thread Network Name.", default='')
    parser.add_argument('--thread_networkkey', help="Thread Network Key", default='')
    parser.add_argument('--thread_xpanid', help="Thread XPAN ID.", default='')
    parser.add_argument('--thread_panid', help="Thread PAN ID.", default='')
    parser.add_argument('--thread_channel', help="Thread Channel", default='')


    args = parser.parse_args()

    """
    Start WeaveDeviceMgr
    """
    wdm = WeaveDeviceManager()
    """
    On Weave Device Manager,
        1. scan ble of device
        2. connect using PASE
        3. add-wifi-network
        4. enable network
        5. create-fabric
        6. register-service
    """

    """
    Read the file for register_service command
    """
    f = open(args.register_firstdevice, "r")
    contents = f.read();
    command = contents.split("weave-register-service generated the service registration command:")
    command1_rs = command[1].strip()
    command1_wat = command[0].split("Weave Access Token:")[1].strip()
    f.close();

    f = open(args.register_seconddevice, "r")
    contents = f.read();
    command = contents.split("weave-register-service generated the service registration command:")
    command2_rs = command[1].strip()
    command2_wat = command[0].split("Weave Access Token:")[1].strip()
    f.close();

    """
    Make sure we have the paring code for the first device
    """
    if args.second_device_only == 'False':

        ble_scan_connect(wdm, args.ble_firstdevice)

        connect(wdm, pase=args.pairingcode_firstdevice)

        """
        arm-fail-safe command
        """
        try:
            output = wdm.chat('arm-fail-safe reset', timeout=60)
        except:
            raise RuntimeError("Process failed at arm-fail-safe reset")
        if 'Arm fail-safe complete' not in output:
            raise RuntimeError("Unable to arm-fail-safe on first device.")

        """
        add-wifi-network command
        """
        network_id = ''
        try:
            output = wdm.chat('add-wifi-network  "' + args.wifi_name + '" ' + args.wifi_security_type + ' ' + args.wifi_password, timeout=60)
            network_id = re.findall(r'network id = (\d*)\)', output)[0].strip()
        except:
            raise RuntimeError("Process failed at add-wifi-network")
        if 'Add wifi network complete' not in output:
            raise RuntimeError("Unable to add wifi network to first device.")

        enable_network(wdm, network_id)

        test_network(wdm, network_id)

        """
        create-fabric command
        """
        try:
            output = wdm.chat('create-fabric', timeout=60)
        except:
            raise RuntimeError("Process failed at create-fabric")
        if 'Create fabric complete' not in output:
            raise RuntimeError("Unable to create-fabric on first device.")

        register_service(wdm, command1_rs)

        """
        disarm-fail-safe command
        """
        try:
            output = wdm.chat('disarm-fail-safe', timeout=60)
        except:
            raise RuntimeError("Process failed at disarm-fail-safe")
        if 'Disarm fail-safe complete' not in output:
            raise RuntimeError("Unable to disarm-fail-safe on first device.")

        close(wdm)

    else:
        if args.first_device_info == 'True':
            ble_scan_connect(wdm, args.ble_firstdevice)

            connect(wdm, case=command1_wat)

            """
            Get-fabric-config command
            """
            fabric_config = ''
            try:
                output = wdm.chat('get-fabric-config')
                fabric_config = re.findall(r'Fabric configuration:\s(.*)\r\n', output)[0].strip()
            except:
                raise RuntimeError("Unable to retrieve fabric configuration.")

            """
            Get networks command
            """
            networks = ''
            try:
                output = wdm.chat('get-networks')
                thread_name = re.findall(r'Name: \"(.*)\"', output)[0].strip()
                thread_key = re.findall(r'Network Key: (.*)', output)[0].strip()
                thread_channel = re.findall(r'Channel: (\d*)', output)[0].strip()
                pan_ids = re.findall(r'PAN Id: (.*)', output)
                thread_xpan_id = pan_ids[0].strip()
                thread_pan_id = pan_ids[1].strip()
                add_thread_network_args = '"' + thread_name + '" ' + thread_xpan_id + " " +  thread_key + " thread-pan-id=" + thread_pan_id + " thread-channel=" + thread_channel
            except:
                raise RuntimeError("Unable to retrieve fabric configuration.")

            close(wdm)

            WaitForInput()

        else:
            thread_name = args.thread_networkname
            thread_key = args.thread_networkkey
            thread_channel = args.thread_channel
            thread_xpan_id = args.thread_xpanid
            thread_pan_id = args.thread_panid

    ble_scan_connect(wdm, args.ble_seconddevice)

    connect(wdm)

    """
    add-thread-network command
    """
    network_id = ''
    try:
        output = wdm.chat('add-thread-network  ' + add_thread_network_args, timeout=60)
        network_id = re.findall(r'network id = (\d*)\)', output)[0].strip()
    except:
        raise RuntimeError("Process failed at add-thread-network")
    if 'Add thread network complete' not in output:
        raise RuntimeError("Unable to add thread network to second device.")

    enable_network(wdm, network_id)

    """
    join-exisiting-fabric
    """
    try:
        output = wdm.chat('join-existing-fabric  ' + fabric_config, timeout=60)
    except:
        raise RuntimeError("Process failed at join-existing-fabric")
    if 'Join existing fabric complete' not in output:
        raise RuntimeError("Unable to join-existing-fabric")

    WaitForInput()

    # test_network(wdm, network_id)

    register_service(wdm, command2_rs)

    close(wdm)

    wdm.close()

if __name__ == '__main__':
    main()