# Asterisk channel driver for *Quectel* and *SimCOM* modules

See original [README](//github.com/IchthysMaranatha/asterisk-chan-quectel/blob/master/README.md) of this project.

----

Supported modules:

* Quectel *EC25-E*,
* SimCOM *SIM7600G-H*,
* SimCOM *SIM900* (analog input/output, see [here](//github.com/RoEdAl/asterisk-chan-quectel/wiki/Using-SIM900-board-with-analog-input-and-output) for more details),
* SimCOM *SIM800C* (analog input/output),
* SimCOM *A7670E* (analog input/output, see [here](//github.com/RoEdAl/asterisk-chan-quectel/wiki/SimCOM-A7670X-limitations) for more details).

# Changes

## General

* Minimal supported Asterisk version is **16** now.
* Using JSON-formatted channel variables.

  One may use [`JSON_DECODE`](https://docs.asterisk.org/Asterisk_21_Documentation/API_Documentation/Dialplan_Functions/JSON_DECODE/) function
  in dialplan in order to extract required field.

  * All `QUECTEL*` variables are wrapped into one JSON-formatted `QUECTEL` variable.
  * For SMS reporting `SMS` JSON-formatted variable is defined.

    ```ini
    [incoming-sms]
    exten => sms,1,NoOp(SMS from device ${JSON_DECODE(QUECTEL,name)})
    same => n,Set(SMS_TXT=${JSON_DECODE(SMS,msg)})
    same => n,GotoIf($[${EXISTS(${SMS_TXT})}]?smstxt:smsempty)
    same => n(smstxt),Verbose(2, [${JSON_DECODE(QUECTEL,name)}] Incoming SMS from ${CALLERID(num)} [${JSON_DECODE(SMS,ts)}]: ${SMS_TXT})
    same => n,Goto(smsbye)
    same => n(smsempty),Verbose(2, [${JSON_DECODE(QUECTEL,name)}] Empty incoming SMS from ${CALLERID(num)})
    same => n(smsbye),Hangup
    ```

  * For USSD rporting `USSD` JSON-formatted variable is defined.

    ```ini
    [incoming-ussd]
    exten => ussd,1,NoOp(USSD from device ${JSON_DECODE(QUECTEL,name)})
    same => n,Set(USSD_TXT=${JSON_DECODE(USSD,ussd)})
    same => n,Verbose(2, [${JSON_DECODE(QUECTEL,name)}] Incoming USSD [${JSON_DECODE(USSD,type_description)}]: ${USSD_TXT})
    same => n,Hangup
    ```

  * For reporting `REPORT` JSON-formatted variable is defined.

    ```ini
    [incoming-report]
    exten => report,1,NoOp(Report from device ${JSON_DECODE(QUECTEL,name)})
    same => n,Set(REPORT_SUBJECT=${JSON_DECODE(REPORT,subject)})
    same => n,Set(REPORT_DIRECTION=${JSON_DECODE(REPORT,direction)})
    same => n,Set(REPORT_SUCCESS=${JSON_DECODE(REPORT,success)})
    same => n,Set(REPORT_JSON=${JSON_DECODE(REPORT,report)})
    same => n,Set(REPORT_INFO=${JSON_DECODE(REPORT_JSON,info)})
    same => n,GotoIf($[${REPORT_SUCCESS} = 1]?reportsuccess:reportfail)
    same => n(reportsuccess),Verbose(2,${JSON_DECODE(QUECTEL,name)} - ${REPORT_SUBJECT} - ${REPORT_DIRECTION} - ${CALLERID(num)} - ${REPORT_INFO})
    same => n,Goto(reportbye)
    same => n(reportfail),Verbose(1,${JSON_DECODE(QUECTEL,name)} - ${REPORT_SUBJECT} - ${REPORT_DIRECTION} - ${CALLERID(num)} - ${REPORT_INFO})
    same => n,Goto(reportbye)
    same => n,Hangup
    ```
  
  * Renamed and reimplemented dialplan functions (applications).

    * `QuectelStatus` application renamed to `QUECTEL_STATUS` **function**.

        ```ini
        same => n,Set(QSTATUS=${QUECTEL_STATUS(quectel0)})
        same => Verbose(2,Device status: ${QSTATUS})
        ```

    * New `QUECTEL_STATUS_EX` **function**.

        Returns device status as JSON.

        ```ini
        same => n,Set(QQSTATUS=${QUECTEL_STATUS_EX(quectel0)})
        same => Verbose(2,Extended device status: ${QQSTATUS})
        ```

    * `QuectelSendSms` application renamed to `QUECTEL_SEND_SMS` one.

        The first parameter of this function may be a *resource string* e.g. `QUECTEL_SEND_SMS(g0,...)`.

    * `QuectelSendUssd` application renamed to `QUECTEL_SEND_USSD` one.

  * Standard `MessageSend` funcion (application) may be used to send SMS-es:

      Use `mobile` techonlogy. `MESSAGE(from)` field is ignored.

      ```ini
      same => n,Set(MESSAGE(body)=This is a short message)
      same => n,Set(MESSAGE(to)=XXXXXXXXX)
      same => n,MessageSend(mobile:quectel0)
      ```

  * Removed autodiscovery feature.

      If you want to access device via `IMEI` or `IMSI` then *udev* rules is a better approach.
      An example you may find in [tools/udev](tools/udev) folder.

## Configuration

* `quectel_uac` option renamed to `uac` and it's a on/**off**/ext switch now.

    `alsadev` option is also defaulted to `hw:Android` (when `uac=on`) or `hw:0` (when `uac=ext`).

* New `multiparty` option (on/**off**).

    Ability to handle multiparty calls wastly complicates audio handling.
    Without multiparty calls audio handling is much simpler and uses less resources (CPU, memory, synchronization objects).
    I decided to turn off multiparty calls support by default.
    You can enable it but remember that multiparty calls were never working in UAC mode and even in TTY (serial) mode this support should be considered as **unstable**.
    When `mutliparty` is *off* all multiparty calls are **actively rejected**.

* `dtmf` option is a on/**off** switch now.

    DTMF detection is now performed by module itself (`AT+QTONEDET` or `AT+DDET` command).

* New `dtmf_duration` option.

    Duration in miliseconds of generated *DTMF*.

* New `msg_direct` option (**none**/on/off).

    Specify how to receive messages (`AT+CNMI` command):

    | value | description |
    | :---: | ------- |
    | **none** | do not change |
    | on | messages are received directly by `+CMT:` *URC*  |
    | off | messages are received by `AT+CMGR` command in response to `+CMTI` *URC* |

* New `msg_storage` option (**auto**/sm/me/mt/sr).

    Setting prefered message storage (`AT+CPMS` command):

    | value | description |
    | :---: | ------- |
    | **auto** | do not change |
    | sm | (U)SIM message storage |
    | me | mobile equipment message storage |
    | mt | same as me storage |
    | sr | SMS status report storage location |

* New `msg_service` option (**-1**/0/1).

    Selecting `Message Service` (`AT+CSMS` command):

    | value | description |
    | :---: | ------ |
    | **-1** | do not change |
    | 0 | SMS AT commands are compatible with *GSM phase 2* |
    | 1 | SMS AT commands are compatible with *GSM phase 2+* |

* New `moh` option (**on**/off).

    Specify hold/unhold action:

    | value  | description |
    | :----: | ----------- |
    | **on** | play/stop MOH |
    | off    | disable/enable uplink voice using `AT+CMUT` command |

* `txgain` and `rxgain` options reimplented.

  * TX/RX gain is performed by module itself.

    This channel driver just sends `AT+QMIC`/`AT+QRXGAIN` (`AT+CMICGAIN`/`AT+COUTGAIN` for *SimCOM* module) commands.

  * Default value is **-1** now - use current module setting, do not change gain.
  * Range: **0-65535** or **0-100%**.

    See also `quectel autio gain tx` and `quectel audio gain rx` commands below.

* New `query_time` option (on/**off**).

    | value | description |
    | :---: | ----------- |
    | on | *ping* module with `AT+QLTS` (*Quectel*) or `AT+CCLK` (*SimCOM*) command |
    | **off** | *ping* module with standard `AT` command |

* New `slin16` option (on/**off**).

    Enable/disable 16kHz audio (default is 8kHz).\
    Currently only *SimCOM* SIM7600X module handles 16kHz audio.

* New `qhup` option (**on**/off).

    For *Quectel* modules hang up calls using `AT+QHUP` (*Hang up Call with a Specific Release Cause*)
    or standard `AT+CHUP` (*Hang up Voice Call*) command.

* New `dsci` option (on/**off**).

   For *Quectel* modules `ccinfo` (`AT+QINDCFG="ccinfo"` command) notifications are used by default.
   You can switch back to (less efficient) `dsci` (`AT^DSCI` command) notifications if your module does not support `ccinfo` notifications.

* Removed `disablesms` option.
* `smsdb` option in `[general]` section defaulted to `:memory:`

    Internal *SQLite3* database is stored in memory by default now.
    You can still put database into a file by specyfying its full path (not recommended).

    See also: [SQLIte3: In-Memory Databases](//www.sqlite.org/inmemorydb.html).

* New `smsdb_backup` option in `[general]` section.

    Path to backup of SMS database created via `quectel sms db backup` command (see below).

* Removed `imsi` and `imei` options.

    Autodiscovery feature was removed (see above),

## Commands

* `simcom` is an alias of `quectel` commands now.

    You may type `simcom…` command instead of `quectel…` one.
    For example `quectel show device status` command is the equivalent of `simcom show device status` one.

* More SMS commands.

  `quectel sms <device> <number> <msg>…` command was renamed to `quectel sms send <device> <number> <msg>…`.

  Additional commands:

  * `quectel sms list received unread <device>`
  * `quectel sms list received read <device>`
  * `quectel sms list all <device>`
  * `quectel sms delete received read <device>`
  * `quectel sms delete all <device>`
  * `quectel sms delete <device> <idx>`
  * `quectel sms direct on <device>`
  * `quectel sms direct off <device>`
  * `quectel sms direct auto <device>`
  * `quectel sms db backup`

    Backup of SMS database. Path to backup file is specified in configuration file via `smsdb_bakup` general option.

* Additional fields in `show device status` command.

  * Added `Access technology`, `Network Name`, `Short Network Name`, `Registered PLMN`, `Band` and `Module Time` fields:

    ```txt
        -------------- Status -------------
    Device                  : quectel0
    State                   : Free
    Audio UAC               : hw:Android
    Data                    : /dev/ttyUSB2
    Voice                   : Yes
    SMS                     : Yes
    Manufacturer            : Quectel
    Model                   : EC25
    Firmware                : EC25EXXXXXXXXXX
    IMEI                    : YYYYYYYYYYYYYYY
    IMSI                    : ZZZZZZZZZZZZZZZ
    GSM Registration Status : Registered, home network
    RSSI                    : 22, -69 dBm
    Access technology       : LTE
    Network Name            : Xxxxx
    Short Network Name      : Yxxxx
    Registered PLMN         : 26006
    Provider Name           : Zxxxxxx
    Band                    : LTE BAND 3
    Location area code      : 0000
    Cell ID                 : AAAAAAA
    Subscriber Number       : Unknown
    SMS Service Center      : +99000111222
    Module Time             : 2000/01/01,00:00:00+08,1
    Tasks in queue          : 0
    Commands in queue       : 0
    Call Waiting            : Disabled
    Current device state    : start
    Desired device state    : start
    When change state       : now
    Calls/Channels          : 0
        Active                : 0
        Held                  : 0
        Dialing               : 0
        Alerting              : 0
        Incoming              : 0
        Waiting               : 0
        Releasing             : 0
        Initializing          : 0
    ```

    Some of theese fields are constantly updated via `act` and `csq` notifications (see `AT+QINDCFG` command).

  * Additional fields in (JSON-formatted) `QUECTEL` variable are defined:

    * `network_name`,
    * `short_network_name`,
    * `privider`,
    * `plmn`,
    * `mcc`,
    * `mnc`,
    * `iccid`.

    `PLMN` (*Public Land Mobile Network Code*) combines `MCC` (*Mobile Country Code*) and `MNC` (*Mobile Network Code*).

  * `SMS Service Center` is now decoded from *UCS-2*.

* Additional fields ins `quectel show device settings` command.

    Displaying new configuration options:

    ````txt
        ------------- Settings ------------
    Device                  : quectel0
    Audio UAC               : hw:Android
    Data                    : /dev/ttyUSB2
    IMEI                    : 
    IMSI                    : 
    Channel Language        : en
    Context                 : quectel-incoming
    Exten                   : 
    Group                   : 0
    RX gain                 : -1
    TX gain                 : -1
    Use CallingPres         : Yes
    Default CallingPres     : Presentation Allowed, Passed Screen
    Disable SMS             : No
    Message Service         : 1
    Message Storage         : ME
    Direct Message          : On
    Auto delete SMS         : Yes
    Reset Quectel           : No
    Call Waiting            : auto
    Multiparty Calls        : No
    DTMF Detection          : No
    Hold/Unhold Action      : Mute
    Query Time              : Yes
    Initial device state    : start
    ````

* New `quectel audio` commands:

    These are just wrappers around few audio-related *AT* commands:

    | command | *AT* command |
    | :------ | ------------ |
    | `quectel audio mode` | `AT+QAUDMOD` |
    | `quectel audio gain tx` | `AT+QMIC` |
    | `quectel audio gain rx` | `AT+QRXGAIN` |
    | `quectel audio loop` | `AT+QAUDLOOP` |

* New `quectel uac apply` command:

    This command just sends `AT+QPCMV=0` and `AT+CFUN=1,1` commands.
    It is helpful if you change `TTY` mode to `UAC` or vice versa.

## Internal

* *UCS-2* encoding is mandatory now.
* Call handling is based on automatic call status contifications.

  * For *Quectel* modules `ccinfo` (default) or `dsci` notifications are used.
  * For *SimCOM* modules `clcc` notifications are used.

* Improved/extended *AT* commands response handler.

    Changes required to properly handle `AT+CMGL` command response.
    This command is now executed at initialization stage in order to receive unread messages.

* Simplyfying audio handling (when `mutliparty` is off, see above) in UAC and TTY mode.

  * Using less resources.
  * Much simpler error handling.
  * More debug messages.
  * Reorganized, improved and simplified code.

* Fixed `USSD` sending/receiving.
* Getting `ICCID` from SIM card.

    `ICCID`: Integrated Circuit Card Identifier.

    It is possible to address device by `ICCID` (`j:` prefix):

    ```ini
    exten => s,n,Dial(Quectel/j:898600700907A6019125/+79139131234)
    ```

* Code (re)formatted by `clang-format` utility.

  Links:

  * [ClangFormat](https://clang.llvm.org/docs/ClangFormat.html),
  * [Edit C++ in Visual Studio Code -- Code formatting](https://code.visualstudio.com/docs/cpp/cpp-ide#_code-formatting).

* Using [`CMake` build system](//github.com/RoEdAl/asterisk-chan-quectel/wiki/Building).
* Improved debug messages.

  * Non-printable characters are C escaped using custom function based on `ast_escape_c`:

    ```log
    DEBUG[11643]: at_read.c:93 at_read: [quectel0] [1][\r\n+QIND: "csq",27,99\r\n]
    DEBUG[11654]: at_read.c:93 at_read: [quectel0] [1][\r\n+CPMS: 0,25,0,25,0,25\r\n\r\nOK\r\n]
    DEBUG[11654]: at_read.c:93 at_read: [quectel0] [1][\r\n+QPCMV: 0,2\r\n\r\nOK\r\n]
    DEBUG[13411]: at_queue.c:181 at_write: [quectel0] [AT+QSPN;+QNWINFO\r]
    ```

  * Using *Unicode* characters in log messages.

    Mostly arrows are used:

    ```log
    DEBUG[20486]: src/at_queue.c:128 at_queue_add: [quectel0][AT+QLTS=1] ↵ [OK][AT+QLTS=1\r] after head
    DEBUG[20486]: src/at_queue.c:336 at_queue_run: [quectel0][AT+QLTS=1] → [AT+QLTS=1\r]
    DEBUG[20486]: src/at_response.c:2718 show_response: [quectel0][AT+QLTS=1] ← [+QLTS][+QLTS: "2000/01/01,00:00:00+00,1"]
    DEBUG[20486]: src/at_response.c:2718 show_response: [quectel0][AT+QLTS=1] ← [OK][OK]
    DEBUG[20486]: src/at_response.c:246 at_response_ok: [quectel0][AT+QLTS=1] ✓
    DEBUG[20486]: src/at_queue.c:72 at_queue_remove: [quectel0][AT+QLTS=1] ↳ [OK] tasks:0 

    ```

* Redesigned SMS database.

    Improved database locking.

* Using modern serial port locking methods:

  * `ioctl(fd, TIOCGEXCL, &locking_status)` and `ioctl(fd, TIOCEXCL)`,
  * `flock(fd, LOCK_EX | LOCK_NB)`.

* Heavy usage of *RAII* variables.

    Using `RAII_VAR` and related macros (variables with *destructors*).

* Using thread pools and task processors:

    Reading and processing modem responses are processed in separate threads.
    You may see basic statistics of used thread pools and task processors via `core show taskprocessors like chan-quectel` command.

    ```txt
    Asterisk*CLI> core show taskprocessors like chan-quectel

    Processor                                                               Processed   In Queue  Max Depth  Low water High water
    chan-quectel/pool                                                           16874          0          1        450        500
    chan-quectel/pool-control                                                   33749          0          1        450        500
    chan-quectel/simcom7600-00000051                                            39171          0          4        360        400

    3 taskprocessors
    ```

* Many small optimizations.
