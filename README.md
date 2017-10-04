## Akonadi resource for Microsoft Exchange Web Services

**THIS REPOSITORY IS NOW OBSOLETE**

**Note**: The Akonadi EWS resource has been merged into KDE PIM and will be
released with KDE PIM 17.12. The code now lives in the ``kdepim-runtime``
repository under ``resources/ews``.

This Akonadi resource provides the ability to use Microsoft Exchange accounts
with KDE PIM applications like KMail and KOrganizer.

It allows to use KDE PIM applications with Exchange mailboxes, for which the
server administrator has not enabled access using standard e-mail protocols
(IMAP, POP3 or SMTP). Additionally since the resource communicates with Exchange
using its native protocol (also used by Microsoft Outlook) it allows to use the
full capabilities of Exchange which are not accessible over IMAP or POP3.

The aim of this project is to allow full management of Exchange mailboxes so
that the KDE PIM suite can become a drop-in replacement for Microsoft Outlook.

Currently the EWS resource is an independent project, but in the long term the
plan is for this to become part of KDE PIM.

### Current status

The EWS resource is currently in preview state. It can be used as a daily driver
when it comes to e-mail. For other tasks like calendar interaction (scheduling
and accepting meetings) it is necessary to use wither OWA or Microsoft Outlook.

### Supported features

* E-mail reception and full mailbox access
* Message & folder operations (copying, moving, deleting)
* Sending e-mail through Exchange (by default Exchange doesn't use SMTP to send
  messages from clients)
* Server-side tags
* Calendar view (read-only)

### Planned features

* Full calendar support (currently only read-only support is provided)
* Task support
* Server-side address autocompletion (will need to sort out some KDE PIM
  limitations)
* Out-of-office status manipulation
* Server-side message filtering
* Access to additional mailboxes & shared calendars

### Software requirements

* Microsoft Exchange 2007 SP1 or later
* Qt 5.5 or later
* KDE Frameworks 5.17 or later (at least 5.19 is recommended when using NTLMv2
  authentication)*
* KDE PIM 15.04.0 or later

\* Alternatively you can also recompile the `kio` package and backport the
following commits from [kio.git](https://quickgit.kde.org/?p=kio.git):

* 5961ac8e Fix NTLMv2 stage 3 response creation
* 2f894291 Try NTLMv2 authentication if the server denies NTLMv1
* f665dd30 Try multiple authentication methods in case of failures

The last commit is only necessary if your system is also configured to use Kerberos.

### Debugging

#### Some Akonadi basics

Akonadi runs every resource in a separate process, which communicates to
the Akonadi server over DBus. When multiple instances of the same resource (for
ex. multiple Exchange accounts) are present, each of them also runs in a
separate process. You can easily find out your EWS resource instances by
running `ps -fu $(whoami) | grep akonadi_ews_resource`.

Each Akonadi resource has a unique identifier. It is composed automatically out
of the resource name and a number, which starts from 0 and is incremented with
every new resource of that type. Removing a resource and adding it again will
not reset the counter. For example the first instance of the EWS resource would
use the identidier `akonadi_ews_resource_0`. The actual resource identifier is
visible in the command line of the resource process after executing the `ps`
command above.

#### Running the resource process in the terminal

By default the resource processes are started by the Akonadi server in the
background. All the output is sent to the log (either the journal in case of
systemd or the ~/.xsession-errors otherwise).

It is possible to run the resource process in the foreground in which case all
the logs will be visible in the active console. Such mode of operation is very
useful for debugging.

To run the resource process interactively:

 1. Learn the resource identifier (use the `ps` command above).
 2. Kill the resource process. This needs to be done a few times as Akonadi will
    initially try to restart the resource. The easiest way is to execute
	`while [ $(killall akonadi_ews_resource) ]; do sleep 1; done`. Warning, if
    you have several instances of the Exchange resource (several accounts) this
    command will terminate all of them.
 3. Execute the resource process interactively (substitute the identifier with
    the one obtained in step 1:
    `akonadi_ews_resource --identifier akonadi_ews_resource_0`.

Once running interactively you can at any moment terminate it by pressing Ctrl+C
and start it again.

In order to run the resource process in the background again you have to restart
Akonadi by executing `akonadictl restart`.

#### Enabling additional debug messages.

Currently there are two kinds of debug messages that can be enabled in addition
to the default ones:

 - Request information (log_ews_resource_request) - prints information about each
   EWS request sent and the response received.
 - Protocol information (log_ews_resource_proto) - dumps all request and
   response data (XML).

To enable these messages edit (or create) the file
`~/.config/QtProject/qtlogging.ini` and put the following lines inside:

```
[Rules]
log_ews_resource_request.debug=true
log_ews_resource_proto.debug=true
```

Setting each of them to `false` or adding a `#` character in front of the line
will disable each of the logging categories.

When starting the resource interactively make sure to edit the file and adjust
the debug messages before actually starting the resource.

#### Request dumps

When the protocol information messages are enabled or when some request fails
the contents of the request and the associated response will be dumped into
files in your `/tmp/` directory. The names of the relevant files will be printed
in the log next to the information about the failed request. Attaching such
dumps can be useful for debugging, however before doing so please make sure they
don't contain any confidential information.

### Reporting bugs

Please report bugs on the GitHub project page. In most cases it will be a great
help when logs are provided.

Note however that the logs may information from your e-mail that might include
company confidential information. Before sending any logs please review them and
make sure any such information like e-mail addresses, dates or subjects is
anonymized (i.e. replaced by some meaningless information).
