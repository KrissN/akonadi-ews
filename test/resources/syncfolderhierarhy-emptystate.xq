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
    <m:SyncState>bNUUPDWHTvuG9p57NGZdhjREdZXDt48a0E1F22yThko=</m:SyncState>
    <m:IncludesLastFolderInRange>true</m:IncludesLastFolderInRange>
    <m:Changes>
    <t:Create>
    <t:Folder>
    <t:FolderId Id="aW5ib3g=" ChangeKey="MDAx" />
    <t:ParentFolderId Id="cm9vdA==" ChangeKey="MDAx" />
    <t:FolderClass>IPF.Note</t:FolderClass>
    <t:TotalCount>1</t:TotalCount>
    <t:DisplayName>Inbox</t:DisplayName>
    <t:ChildFolderCount>0</t:ChildFolderCount>
    <t:UnreadCount>0</t:UnreadCount>
    </t:Folder>
    </t:Create>
    <t:Create>
    <t:CalendarFolder>
    <t:FolderId Id="Y2FsZW5kYXI=" ChangeKey="MDAx" />
    <t:ParentFolderId Id="cm9vdA==" ChangeKey="MDAx" />
    <t:FolderClass>IPF.Calendar</t:FolderClass>
    <t:TotalCount>0</t:TotalCount>
    <t:DisplayName>Calendar</t:DisplayName>
    <t:ChildFolderCount>0</t:ChildFolderCount>
    </t:CalendarFolder>
    </t:Create>
    <t:Create>
    <t:TasksFolder>
    <t:FolderId Id="dGFza3M=" ChangeKey="MDAx" />
    <t:ParentFolderId Id="cm9vdA==" ChangeKey="MDAx" />
    <t:FolderClass>IPF.Tasks</t:FolderClass>
    <t:TotalCount>0</t:TotalCount>
    <t:DisplayName>Tasks</t:DisplayName>
    <t:ChildFolderCount>0</t:ChildFolderCount>
    </t:TasksFolder>
    </t:Create>
    <t:Create>
    <t:ContactsFolder>
    <t:FolderId Id="Y29udGFjdHM=" ChangeKey="MDAx" />
    <t:ParentFolderId Id="cm9vdA==" ChangeKey="MDAx" />
    <t:FolderClass>IPF.Contacts</t:FolderClass>
    <t:TotalCount>0</t:TotalCount>
    <t:DisplayName>Contacts</t:DisplayName>
    <t:ChildFolderCount>0</t:ChildFolderCount>
    </t:ContactsFolder>
    </t:Create>
    </m:Changes>
    </m:SyncFolderHierarchyResponseMessage>
    </m:ResponseMessages>
    </m:SyncFolderHierarchyResponse></soap:Body></soap:Envelope>
) else ()