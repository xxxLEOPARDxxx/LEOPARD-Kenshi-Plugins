 // Controller guards/cancellation code below is extracted from production.
 CohortControl c(w);InterlockedExchange(&w.state,WIdle);
 c.mainThread=false;C(!c.DrainCompleted(),"foreign thread cannot retain authenticated source");c.mainThread=true;
 c.state_=CohortControl::Complete;C(!c.DrainCompleted(),"borrowable Complete cannot trim");c.state_=CohortControl::Idle;
 w.accepted.ready=true;C(!c.DrainCompleted(),"accepted consumer must release before trim");w.accepted.Clear();
 c.queryPending_=true;C(!c.DrainCompleted(),"pending GPU query cannot enter completed trim");c.queryPending_=false;
 InterlockedExchange(&w.state,WBusy);C(!c.DrainCompleted(),"busy source worker cannot enter completed trim");InterlockedExchange(&w.state,WIdle);
 C(c.DrainCompleted(),"acknowledged Idle admits completed-pair drain");
 c.fenceResult=S_FALSE;C(c.PumpCancel()==S_FALSE&&!c.closeSent_&&!c.cleared,"GPU fence blocks trim and GPU clear");c.fenceResult=S_OK;
 C(c.PumpCancel()==S_FALSE&&c.trimSent_,"ordinary GPU drain dispatches Trim");
 DWORD until=GetTickCount();while(w.state==WBusy&&GetTickCount()-until<2000)Sleep(1);
 C(c.PumpCancel()==S_OK&&c.state_==CohortControl::Idle&&c.cleared==1,"ordinary trim clears GPU before returning Idle");
 C(w.height.Ready()&&w.window.bytes.capacity()==0&&w.pixels.capacity()==0,"completed drain retains handle only");
 // Hard cancellation after Trim was dispatched must wait for the Trim ack,
 // then send real Close before it can report Idle or release GPU owners.
 c.gpu.bank.bytes=1;C(c.DrainCompleted(),"second completed-pair trim admitted");C(c.PumpCancel()==S_FALSE&&c.trimSent_,"second Trim dispatched");c.Cancel();
 until=GetTickCount();while(w.state==WBusy&&GetTickCount()-until<2000)Sleep(1);
 C(c.PumpCancel()==S_FALSE&&!c.trimSent_&&c.cleared==1,"hard cancel upgrades dispatched Trim without early GPU clear");
 until=GetTickCount();while(w.state==WBusy&&GetTickCount()-until<2000)Sleep(1);
 C(c.PumpCancel()==S_OK&&!w.height.Ready()&&!WriteDenied(b)&&c.cleared==2,"hard cancel awaits full Close acknowledgment");
 // A context cancellation before command dispatch must select Close directly.
 ResetEvent(w.cancel);Send(w,Height,WHeight);InterlockedExchange(&w.state,WIdle);c.gpu.bank.bytes=1;
 C(c.DrainCompleted(),"context control starts completed drain");c.Cancel();C(c.PumpCancel()==S_FALSE&&!c.trimSent_,"context cancellation before dispatch selects full Close");
 until=GetTickCount();while(w.state==WBusy&&GetTickCount()-until<2000)Sleep(1);
 C(c.PumpCancel()==S_OK&&!w.height.Ready()&&c.cleared==3,"context full Close removes retained authentication");
 C(c.DrainCompleted()&&c.state_==CohortControl::Idle,"already empty Idle drain is a successful no-op");
 ResetEvent(w.cancel);Send(w,Height,WHeight);
