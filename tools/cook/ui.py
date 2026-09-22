"""Cook bounded UI records and shaped text through the existing font/texture stack."""
import io
import json
import re
import struct
from PIL import Image, ImageDraw, ImageFont, features
from model import wrapped
from weapon import text
import texture

KINDS = ('label','button','slider','binding','value')
ACTIONS = ('none','page','resume','map','quit')
BINDINGS = ('+attack','+forward','+back','+moveleft','+moveright','+moveup','+movedown','+speed','+voiprecord','weapnext','weapprev')
CVARS = {'s_volume':(0,1),'s_musicvolume':(0,1),'s_hrtf':(0,1),'sensitivity':(.1,30),'cl_run':(0,1)}
VALUES = ('health','armor','ammo','ping','fps')


def unique(rows, label):
    names=[row['id'] for row in rows]
    if len(names)!=len(set(names)):
        raise ValueError('duplicate UI '+label+' id')
    return {name:index for index,name in enumerate(names)}


def cook(source,name,read):
    definition=json.loads(read(source))
    if not features.check('raqm'):
        raise ValueError('UI text requires Pillow with FreeType/RAQM shaping')
    font_data=read(source.parent/definition['font'])
    pages,labels,locales=(definition[key] for key in ('pages','texts','locales'))
    page_ids=unique(pages,'page')
    text_ids=unique(labels,'text')
    locale_ids=unique(locales,'locale')
    if not {'main','options','hud'}<=page_ids.keys():
        raise ValueError('UI requires main, options and hud pages')
    items=[]
    page_records=bytearray()
    for page in pages:
        unique(page['items'],'item')
        page_records.extend(struct.pack('<32sII',text(page['id'],32),len(items),len(page['items'])))
        items.extend(page['items'])
    if len(items)>128:
        raise ValueError('UI exceeds 128 items')
    item_records=bytearray()
    for row in items:
        kind=row['kind']
        action=row.get('action','none')
        target=row.get('target','')
        bounds=row.get('range',[0,0,0])
        if row['text'] not in text_ids:
            raise ValueError('UI item references missing text')
        if row['rect'][2]<=0 or row['rect'][3]<=0:
            raise ValueError('UI rectangle requires positive size')
        if kind=='button':
            if action=='none' or (action=='page' and target not in page_ids):
                raise ValueError('UI button requires a valid action/page')
            if action=='map' and not re.fullmatch(r'[a-z0-9_-]+',target):
                raise ValueError('UI map action requires a map basename')
            if action in ('resume','quit') and target:
                raise ValueError('UI resume/quit action takes no target')
        elif action!='none':
            raise ValueError('only UI buttons have actions')
        if kind=='slider':
            limits=CVARS.get(target)
            if not limits or not limits[0]<=bounds[0]<bounds[1]<=limits[1] or not 0<bounds[2]<=bounds[1]-bounds[0]:
                raise ValueError('UI slider requires a supported cvar and bounded range/step')
        elif 'range' in row:
            raise ValueError('only UI sliders have a range')
        if kind=='binding' and target not in BINDINGS:
            raise ValueError('UI binding command is unsupported')
        if kind=='value' and target not in VALUES:
            raise ValueError('UI HUD value is unsupported')
        if kind=='label' and target:
            raise ValueError('UI labels have no target')
        item_records.extend(struct.pack('<32sIII64s4f2f4f3f',text(row['id'],32),KINDS.index(kind),
            ACTIONS.index(action),text_ids[row['text']],target.encode('ascii'),*row['rect'],*row['anchor'],
            *row.get('color',[1,1,1,1]),*bounds))
    # Shaped runs, then printable key/numeric glyphs. No runtime shaping/allocation.
    fonts={}
    bitmaps=[]
    def shape(value,size,direction,language):
        value.encode('utf-8')
        if size not in fonts:
            fonts[size]=ImageFont.truetype(io.BytesIO(font_data),size*2,layout_engine=ImageFont.Layout.RAQM)
        font=fonts[size]
        options=dict(direction=direction,language=language,anchor='ls')
        left,top,right,bottom=font.getbbox(value,**options)
        width,height=max(1,right-left),max(1,bottom-top)
        if width>2044 or height>2044:
            raise ValueError('UI text exceeds atlas dimensions; shorten or split the authored label')
        bitmap=Image.new('RGBA',(width,height),(255,255,255,0))
        ImageDraw.Draw(bitmap).text((-left,-top),value,font=font,fill=(255,255,255,255),**options)
        advance=font.getlength(value,direction=direction,language=language)/2
        bitmaps.append((bitmap,left/2,top/2,advance))
    for locale in locales:
        for label in labels:
            if set(label['values'])!=locale_ids.keys():
                raise ValueError('every UI text requires exactly the declared locales')
            shape(label['values'][locale['id']],label['size'],locale['direction'],locale['id'])
    for code in range(32,127):
        shape(chr(code),32,'ltr','en')
    width=2048
    x=y=2
    row_height=0
    packed=[]
    for bitmap,left,top,advance in bitmaps:
        w,h=bitmap.size
        if w+4>width or h+4>2048:
            raise ValueError('UI text exceeds atlas dimensions; shorten or split the authored label')
        if x+w+2>width:
            x=2
            y+=row_height+4
            row_height=0
        if y+h+2>2048:
            raise ValueError('UI text atlas exceeds 2048 square pixels')
        packed.append((x,y,w,h,left,top,advance))
        x+=w+4
        row_height=max(row_height,h)
    height=1<<(y+row_height+1).bit_length()
    atlas=Image.new('RGBA',(width,height),(255,255,255,0))
    for (bitmap,*_),(x,y,*_) in zip(bitmaps,packed):
        atlas.paste(bitmap,(x,y))
    atlas_name=name+'-font.ktx2'
    header=struct.pack('<32s64s2f7I',text(definition['name'],32),text(atlas_name),*definition['canvas'],
                       len(pages),len(items),len(labels),len(locales),95,width,height)
    payload=bytearray(header+page_records+item_records)
    for row in labels:
        payload.extend(struct.pack('<32sf',text(row['id'],32),row['size']))
    for row in locales:
        payload.extend(struct.pack('<16sI',text(row['id'],16),int(row['direction']=='rtl')))
    for row in packed:
        payload.extend(struct.pack('<4I3f',*row))
    image=io.BytesIO()
    atlas.save(image,format='PNG')
    return {name+'.asui':wrapped(b'ASUI\0\0\0\0',payload),
            atlas_name:texture.cook(image.getvalue(),dict(format='bc7',srgb=False))}
