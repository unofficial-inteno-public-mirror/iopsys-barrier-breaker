//! Author: Martin K. Schröder <mkschreder.uk@gmail.com>

JUCI.app
.controller("StatusVoicePageCtrl", function($scope, $rpc){
	$rpc.asterisk.status().done(function(data){
		if(data && data.sip){
			var accounts = []; 
			Object.keys(data.sip).map(function(k){
				if(data.sip[k].ip) accounts.push(data.sip[k]); 
			}); 
			$scope.sipAccounts = accounts; 
			$scope.$apply(); 
		}
	}); 
}); 
