declare namespace t = "http://schemas.microsoft.com/exchange/services/2006/types";
declare namespace m = "http://schemas.microsoft.com/exchange/services/2006/messages";
declare namespace soap = "http://schemas.xmlsoap.org/soap/envelope/";
if (/soap:Envelope/soap:Body/m:GetFolder and 
    //m:GetFolder/m:FolderShape/t:BaseShape = <t:BaseShape>IdOnly</t:BaseShape> and 
    not(//t:AdditionalProperties) and 
    count(//m:GetFolder/m:FolderIds/t:DistinguishedFolderId) = 5 and 
    count(//m:GetFolder/m:FolderIds/t:FolderId) = 0 and 
    //m:GetFolder/m:FolderIds/t:DistinguishedFolderId[position()=1 and @Id="inbox"] and 
    //m:GetFolder/m:FolderIds/t:DistinguishedFolderId[position()=2 and @Id="outbox"] and 
    //m:GetFolder/m:FolderIds/t:DistinguishedFolderId[position()=3 and @Id="sentitems"] and 
    //m:GetFolder/m:FolderIds/t:DistinguishedFolderId[position()=4 and @Id="deleteditems"] and 
    //m:GetFolder/m:FolderIds/t:DistinguishedFolderId[position()=5 and @Id="drafts"]
) then (
    <soap:Envelope><soap:Header>
    <t:ServerVersionInfo MajorVersion="15" MinorVersion="01" MajorBuildNumber="225" MinorBuildNumber="042" />
    </soap:Header><soap:Body>
    <m:GetFolderResponse>
    <m:ResponseMessages>
    %1
    </m:ResponseMessages>
    </m:GetFolderResponse></soap:Body></soap:Envelope>
) else ()