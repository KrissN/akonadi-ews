declare namespace t = "http://schemas.microsoft.com/exchange/services/2006/types";
declare namespace m = "http://schemas.microsoft.com/exchange/services/2006/messages";
declare namespace soap = "http://schemas.xmlsoap.org/soap/envelope/";
if (/soap:Envelope/soap:Body/m:Subscribe/m:StreamingSubscriptionRequest and
    count(//m:StreamingSubscriptionRequest/t:FolderIds/t:FolderId) = 4 and
    //m:StreamingSubscriptionRequest/t:FolderIds/t:FolderId[@Id="aW5ib3g="] and
    //m:StreamingSubscriptionRequest/t:FolderIds/t:FolderId[@Id="Y2FsZW5kYXI="] and
    //m:StreamingSubscriptionRequest/t:FolderIds/t:FolderId[@Id="dGFza3M="] and
    //m:StreamingSubscriptionRequest/t:FolderIds/t:FolderId[@Id="Y29udGFjdHM="] and
    count(//m:StreamingSubscriptionRequest/t:EventTypes/t:EventType) = 6
) then (
    <soap:Envelope><soap:Header>
    <t:ServerVersionInfo MajorVersion="15" MinorVersion="01" MajorBuildNumber="225" MinorBuildNumber="042" />
    </soap:Header><soap:Body>
    <m:SubscribeResponse>"
    <m:ResponseMessages>"
    <m:SubscribeResponseMessage ResponseClass="Success">
    <m:ResponseCode>NoError</m:ResponseCode>
    <m:SubscriptionId>otPO1iOPzS+vL5HOuTZY+cCfIIn+vRqOO0ZHTEWAa8k=</m:SubscriptionId>
    </m:SubscribeResponseMessage>
    </m:ResponseMessages>
    </m:SubscribeResponse></soap:Body></soap:Envelope>
) else ()