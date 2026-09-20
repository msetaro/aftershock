package main

import (
 "os"
 "path/filepath"
 "strings"
 "testing"
)

const goodSpec = `{"id":"local-1","map":"two_lane","mode":0,"frag_limit":1,"time_limit":1,"players":2,"password":"local-secret","token":"allocation-secret"}`

func TestSpec(t *testing.T) {
 s,err:=decodeSpec([]byte(goodSpec)); if err!=nil {t.Fatal(err)}
 if s.Map!="two_lane" || s.Players!=2 {t.Fatal(s)}
 for _,bad:=range []string{
  strings.Replace(goodSpec,`"two_lane"`,`"two_lane;quit"`,1),
  strings.Replace(goodSpec,`"players":2`,`"players":0`,1),
  strings.Replace(goodSpec,`"mode":0`,`"mode":99`,1),
  strings.Replace(goodSpec,`"password":"local-secret"`,`"password":""`,1),
  strings.Replace(strings.Replace(goodSpec,`"frag_limit":1`,`"frag_limit":0`,1),`"time_limit":1`,`"time_limit":0`,1),
  strings.Replace(goodSpec,`"token":"allocation-secret"`,`"extra":true,"token":"allocation-secret"`,1),
  goodSpec+`{}`,strings.Repeat(" ",65537)+goodSpec,
 } {if _,err:=decodeSpec([]byte(bad));err==nil {t.Fatalf("accepted invalid spec: %.200s",bad)}}
}

func TestOwnedArguments(t *testing.T) {
 s,_:=decodeSpec([]byte(goodSpec))
 a,err:=serverArgs(s,"/content","/home/match","aftershock",27960,false)
 if err!=nil {t.Fatal(err)}
 text:=strings.Join(a," ")
 for _,want:=range []string{"+set fs_basegame aftershock","+map two_lane","+set sv_exitOnMatchEnd 1","+set g_password local-secret"} {
  if !strings.Contains(text,want) {t.Fatal(text)}
 }
 if _,err:=serverArgs(s,"/content","/home/match","../baseq3",27960,false);err==nil {t.Fatal("accepted invalid content name")}
}

func TestCheckpoint(t *testing.T) {
 c:=checkpoint{}
 for _,line:=range []string{"  0:00 ClientConnect: 1","  0:01 ClientBegin: 1","  0:02 Kill: 1 2 7: Player killed Other by MOD_ROCKET", "  1:00 score: 3  ping: 5  client: 1 Player","  1:00 Exit: Fraglimit hit.","  1:05 ShutdownGame:"} {c.add(line)}
 if c.Kills!=1 || c.Joins!=1 || c.Scores["1"]!=3 || !c.Completed {t.Fatal(c)}
}

func TestIngestDurability(t *testing.T) {
 path:=filepath.Join(t.TempDir(),"events.jsonl")
 s,err:=newIngest(path,map[string]string{"local-1":"allocation-secret"}); if err!=nil {t.Fatal(err)}
 b:=batch{Version:1,Match:"local-1",Start:0,End:21,Events:[]string{"  0:00 InitGame: test"}}
 if err=s.accept(b,"wrong");err==nil {t.Fatal("wrong token accepted")}
 if err=s.accept(b,"allocation-secret");err!=nil {t.Fatal(err)}
 if err=s.accept(b,"allocation-secret");err!=nil {t.Fatal(err)}
 s.file.Close()
 s,err=newIngest(path,map[string]string{"local-1":"allocation-secret"});if err!=nil {t.Fatal(err)}
 defer s.file.Close()
 if err=s.accept(b,"allocation-secret");err!=nil {t.Fatal(err)}
 b.Start=99;b.End=100
 if err=s.accept(b,"allocation-secret");err==nil {t.Fatal("accepted a gap")}
 data,_:=os.ReadFile(path)
 if strings.Count(string(data),"\n")!=1 {t.Fatalf("duplicate durable write: %s",data)}
}
