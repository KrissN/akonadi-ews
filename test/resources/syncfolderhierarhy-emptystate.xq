declare namespace t = "http://schemas.microsoft.com/exchange/services/2006/types";
declare namespace m = "http://schemas.microsoft.com/exchange/services/2006/messages";
declare namespace soap = "http://schemas.xmlsoap.org/soap/envelope/";
if (/soap:Envelope/soap:Body/m:SyncFolderHierarchy and
    //m:SyncFolderHierarchy/m:FolderShape/t:BaseShape = <t:BaseShape>Default</t:BaseShape> and
    count(//t:AdditionalProperties/t:ExtendedFieldURI) = 1 and
    //t:AdditionalProperties/t:ExtendedFieldURI[@PropertyTag="0x3613" and
        @PropertyType="String"] and
    count(//t:AdditionalProperties/t:FieldURI) = 2 and
    //t:AdditionalProperties/t:FieldURI[@FieldURI="folder:EffectiveRights"] and
    //t:AdditionalProperties/t:FieldURI[@FieldURI="folder:ParentFolderId"] and
    count(//m:SyncFolderHierarchy/m:SyncFolderId/t:DistinguishedFolderId) = 1 and
    count(//m:SyncFolderHierarchy/m:SyncFolderId/t:FolderId) = 0 and
    //m:SyncFolderHierarchy/m:SyncFolderId/t:DistinguishedFolderId[@Id="msgfolderroot"]
) then (
    <soap:Envelope><soap:Header>
    <t:ServerVersionInfo MajorVersion="15" MinorVersion="01" MajorBuildNumber="225" MinorBuildNumber="042" />
    </soap:Header><soap:Body>
    <m:SyncFolderHierarchyResponse>
    <m:ResponseMessages>
    <m:SyncFolderHierarchyResponseMessage ResponseClass="Success">
    <m:ResponseCode>NoError</m:ResponseCode>
    <m:SyncState>%1</m:SyncState>
    <m:IncludesLastFolderInRange>true</m:IncludesLastFolderInRange>
    <m:Changes>
    %2
    </m:Changes>
    </m:SyncFolderHierarchyResponseMessage>
    </m:ResponseMessages>
    </m:SyncFolderHierarchyResponse></soap:Body></soap:Envelope>
) else ()