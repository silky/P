// P semantics test: one machine, inherited handler vs deferral of event:
//

event E1 assert 2;
event unit assert 1;

main machine Real1 {
    var test: bool;  //init with "false"
    start state Real1_Init {
        entry { 
			raise unit;
			//send this, E1;
        }
		on unit do {send this, E1; push Real1_S1; };  
        on E1 do Action2;   //Action2 handler for E1 is inherited by Real1_S1	
        exit { send this, E1;  }
	}
	state Real1_S1 {
		entry {
			test = true;
		}
		//deferral of E1 overrides inherited handler:
		defer E1;
    }
	fun Action2() {
		assert(test == false);  //unreachable
    }
}