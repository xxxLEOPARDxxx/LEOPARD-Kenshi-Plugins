"""Explicit Steam client IPC for uploader-only use when legacy auto-init fails.

Uses the installed client's exported pipe/user functions. No registry changes,
client launch or credential access. Completion is polled via ISteamUtils; there
are no registered application callback handlers in this command-line uploader.
"""
import ctypes as c
import os
from pathlib import Path
import steam_upload as u

class Steam(u.Steam):
    def __init__(self):
        os.chdir(u.KENSHI)
        os.environ['SteamAppId']=str(u.APPID);os.environ['SteamGameId']=str(u.APPID)
        root=Path('C:/Program Files (x86)/Steam')
        self.directory=os.add_dll_directory(str(root))
        self.native=c.CDLL(str(root/'steamclient64.dll'))
        self.dll=c.CDLL(str(Path(u.KENSHI)/'steam_api64.dll'))
        def fn(d,name,result,*args):
            f=getattr(d,name);f.restype=result;f.argtypes=list(args);return f
        ptr=c.c_void_p;i=c.c_int;b=c.c_bool;s=c.c_char_p;n=c.c_uint32;q=c.c_uint64
        self._release_user=fn(self.native,'Steam_ReleaseUser',None,i,i)
        self._release_pipe=fn(self.native,'Steam_BReleaseSteamPipe',b,i)
        self.pipe=0;self.user=0
        try:
            self.client=fn(self.native,'CreateInterface',ptr,s,ptr)(b'SteamClient017',None)
            if not self.client:raise RuntimeError('Steam client interface unavailable')
            self.pipe=fn(self.native,'Steam_CreateSteamPipe',i)()
            if not self.pipe:raise RuntimeError('Steam IPC pipe unavailable')
            self.user=fn(self.native,'Steam_ConnectToGlobalUser',i,i)(self.pipe)
            if not self.user:raise RuntimeError('No logged-in Steam user')
            self.SteamClient=lambda:self.client;self.HUser=lambda:self.user;self.HPipe=lambda:self.pipe
            self.ugc=fn(self.dll,'SteamAPI_ISteamClient_GetISteamUGC',ptr,ptr,i,i,s)(self.client,self.user,self.pipe,b'STEAMUGC_INTERFACE_VERSION008')
            self.utils=fn(self.dll,'SteamAPI_ISteamClient_GetISteamUtils',ptr,ptr,i,s)(self.client,self.pipe,b'SteamUtils008')
            if not self.ugc or not self.utils:raise RuntimeError('Steam UGC/Utils unavailable')
            self.IsDone=fn(self.dll,'SteamAPI_ISteamUtils_IsAPICallCompleted',b,ptr,q,c.POINTER(b))
            self.GetResult=fn(self.dll,'SteamAPI_ISteamUtils_GetAPICallResult',b,ptr,q,ptr,i,i,c.POINTER(b))
            self.FailReason=fn(self.dll,'SteamAPI_ISteamUtils_GetAPICallFailureReason',i,ptr,q)
            self.RunCallbacks=lambda:None
            definitions={
                'CreateItem':(q,ptr,n,i),'StartItemUpdate':(q,ptr,n,q),
                'SetItemTitle':(b,ptr,q,s),'SetItemDescription':(b,ptr,q,s),
                'SetItemUpdateLanguage':(b,ptr,q,s),'SetItemVisibility':(b,ptr,q,i),
                'SetItemTags':(b,ptr,q,c.POINTER(u.ParamStringArray)),
                'SetItemContent':(b,ptr,q,s),'SetItemPreview':(b,ptr,q,s),
                'AddItemPreviewFile':(b,ptr,q,s,i),'SubmitItemUpdate':(q,ptr,q,s),
                'GetItemUpdateProgress':(i,ptr,q,c.POINTER(q),c.POINTER(q)),
                'GetNumSubscribedItems':(n,ptr)}
            aliases={'StartItemUpdate':'StartUpdate','SetItemTitle':'SetTitle','SetItemDescription':'SetDescription','SetItemUpdateLanguage':'SetLanguage','SetItemVisibility':'SetVisibility','SetItemTags':'SetTags','SetItemContent':'SetContent','SetItemPreview':'SetPreview','AddItemPreviewFile':'AddPreviewFile','SubmitItemUpdate':'Submit','GetItemUpdateProgress':'Progress','GetNumSubscribedItems':'NumSubscribed'}
            for name,types in definitions.items():setattr(self,aliases.get(name,name),fn(self.dll,'SteamAPI_ISteamUGC_'+name,*types))
        except BaseException:
            self.Shutdown();raise
    def Shutdown(self):
        if self.user:self._release_user(self.pipe,self.user);self.user=0
        if self.pipe:self._release_pipe(self.pipe);self.pipe=0
        self.directory.close()
