declare namespace t = "http://schemas.microsoft.com/exchange/services/2006/types";
declare namespace m = "http://schemas.microsoft.com/exchange/services/2006/messages";
declare namespace soap = "http://schemas.xmlsoap.org/soap/envelope/";
if (/soap:Envelope/soap:Body/m:GetFolder and
    //m:GetFolder/m:FolderShape/t:BaseShape = <t:BaseShape>IdOnly</t:BaseShape> and
    //t:AdditionalProperties/t:FieldURI[@FieldURI="folder:DisplayName"] and
    count(//t:AdditionalProperties/t:ExtendedFieldURI) = 2 and
    //t:AdditionalProperties/t:ExtendedFieldURI[@PropertySetId="9bf757ae-69b5-4d8a-bf1d-2dd0c0871a28" and
        @PropertyName="GlobalTags" and
        @PropertyType="StringArray"] and
    //t:AdditionalProperties/t:ExtendedFieldURI[@PropertySetId="9bf757ae-69b5-4d8a-bf1d-2dd0c0871a28" and
        @PropertyName="GlobalTagsVersion" and
        @PropertyType="Integer"] and
    count(//m:GetFolder/m:FolderIds/t:DistinguishedFolderId) = 2 and
    count(//m:GetFolder/m:FolderIds/t:FolderId) = 0 and
    //m:GetFolder/m:FolderIds/t:DistinguishedFolderId[position()=1 and @Id="msgfolderroot"] and
    //m:GetFolder/m:FolderIds/t:DistinguishedFolderId[position()=2 and @Id="inbox"]
) then (
    <soap:Envelope><soap:Header>
    <t:ServerVersionInfo MajorVersion="15" MinorVersion="01" MajorBuildNumber="225" MinorBuildNumber="042" />
    </soap:Header><soap:Body>
    <m:GetFolderResponse>
    <m:ResponseMessages>
    <m:GetFolderResponseMessage ResponseClass="Success">
    <m:ResponseCode>NoError</m:ResponseCode>
    <m:Folders>
    <t:Folder>
    <t:FolderId Id="cm9vdA==" ChangeKey="MDAx" />
    <t:DisplayName>Root</t:DisplayName>
    </t:Folder>
    </m:Folders>
    </m:GetFolderResponseMessage>
    <m:GetFolderResponseMessage ResponseClass="Success">
    <m:ResponseCode>NoError</m:ResponseCode>
    <m:Folders>
    <t:Folder>
    <t:FolderId Id="aW5ib3g=" ChangeKey="MDAx" />
    <t:DisplayName>Inbox</t:DisplayName>
    </t:Folder>
    </m:Folders>
    </m:GetFolderResponseMessage>
    </m:ResponseMessages>
    </m:GetFolderResponse></soap:Body></soap:Envelope>
) else ()