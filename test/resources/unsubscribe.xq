declare namespace t = "http://schemas.microsoft.com/exchange/services/2006/types";
declare namespace m = "http://schemas.microsoft.com/exchange/services/2006/messages";
declare namespace soap = "http://schemas.xmlsoap.org/soap/envelope/";
if (/soap:Envelope/soap:Body/m:Unsubscribe/m:SubscriptionId = <m:SubscriptionId>otPO1iOPzS+vL5HOuTZY+cCfIIn+vRqOO0ZHTEWAa8k=</m:SubscriptionId>
) then (
    <soap:Envelope><soap:Header>
    <t:ServerVersionInfo MajorVersion="15" MinorVersion="01" MajorBuildNumber="225" MinorBuildNumber="042" />
    </soap:Header><soap:Body>
    <m:UnsubscribeResponse>"
    <m:ResponseMessages>"
    <m:UnsubscribeResponseMessage ResponseClass="Success">
    <m:ResponseCode>NoError</m:ResponseCode>
    </m:UnsubscribeResponseMessage>
    </m:ResponseMessages>
    </m:UnsubscribeResponse></soap:Body></soap:Envelope>
) else ()