"""Read Steam's public API/CDN to verify the exact preview-only publication."""
import concurrent.futures,hashlib,io,json,pathlib,sys,urllib.parse,urllib.request
from PIL import Image
root=pathlib.Path(r'D:/DEV/Kenshi/workshop/steam-covers')
staged=json.loads((root/'staged-previews.json').read_text(encoding='utf-8'))
form=[('itemcount',str(len(staged)))]+[('publishedfileids[%d]'%i,str(c['id'])) for i,c in enumerate(staged)]
request=urllib.request.Request('https://api.steampowered.com/ISteamRemoteStorage/GetPublishedFileDetails/v1/',data=urllib.parse.urlencode(form).encode(),method='POST')
with urllib.request.urlopen(request,timeout=40) as response:payload=json.load(response)
details={str(c['publishedfileid']):c for c in payload['response']['publishedfiledetails']}
for c in staged:
    d=details[str(c['id'])]
    if d['result']!=1 or str(d['creator'])!='76561198098571264' or int(d['consumer_app_id'])!=233860:raise ValueError('Unexpected published item: '+c['mod'])
if sys.argv[1]=='baseline':
    (root/'remote-before.json').write_text(json.dumps(details,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print('Captured metadata for',len(details),'published items before preview-only update.')
else:
    baseline=json.loads((root/'remote-before.json').read_text(encoding='utf-8'))
    def verify(c):
        d=details[str(c['id'])];old=baseline[str(c['id'])]
        unchanged={key:d.get(key)==old.get(key) for key in ('title','description','file_size','file_url','hcontent_file','visibility','tags')}
        if not all(unchanged.values()):raise ValueError('Unexpected non-preview changes: '+c['mod']+' '+str(unchanged))
        url=d['preview_url'];host=urllib.parse.urlsplit(url).hostname or ''
        if urllib.parse.urlsplit(url).scheme!='https' or not (host.endswith('.steamusercontent.com') or host.endswith('.akamaihd.net')):raise ValueError('Unexpected image host')
        with urllib.request.urlopen(url,timeout=40) as response:data=response.read(2*1024*1024)
        sha=hashlib.sha256(data).hexdigest()
        with Image.open(io.BytesIO(data)) as image:size=[image.width,image.height]
        if sha!=c['sha256'] or size!=c['dimensions'] or size[0]!=size[1]:raise ValueError('Preview mismatch: '+c['mod'])
        return dict(mod=c['mod'],id=c['id'],preview_url=url,dimensions=size,download_sha256=sha,preview_matches=True,non_preview_fields_unchanged=unchanged)
    with concurrent.futures.ThreadPoolExecutor(max_workers=6) as pool:results=list(pool.map(verify,staged))
    (root/'remote-verification.json').write_text(json.dumps(results,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print('Verified',len(results),'exact square previews on Steam CDN; content and descriptions unchanged.')
