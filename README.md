## Akonadi resource for Microsoft Exchange Web Services

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
  authentication)
* KDE PIM 15.04.0 or later

### Reporting bugs

Please report bugs on the GitHub project page. In most cases it will be a great
help when logs are provided.

Note however that the logs may information from your e-mail that might include
company confidential information. Before sending any logs please review them and
make sure any such information like e-mail addresses, dates or subjects is
anonymized (i.e. replaced by some meaningless information).
