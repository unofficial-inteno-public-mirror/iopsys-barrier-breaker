$juci.module("wifi")
.controller("WifiWPSPageCtrl", function($scope, $uci, $rpc){
	console.log("WIFI PAGE CONTROLLER"); 
	$scope.data = {
		userPIN: ""
	}
	$uci.sync(["wireless", "boardpanel"]).done(function(){
		if($uci.boardpanel == undefined) $scope.$emit("error", "Boardpanel config is not present on this system!"); 
		else $scope.boardpanel = $uci.boardpanel; 
		if(!$uci.boardpanel.settings){
			$uci.boardpanel.create({".type": "settings", ".name": "settings"}).done(function(section){
				$uci.save(); 
			}).fail(function(){
				$scope.$emit("error", "Could not create required section boardpanel.settings in config!"); 
			}); 
		} 
		$scope.wireless = $uci.wireless; 
		$scope.$apply(); 
	}).fail(function(err){
		console.log("failed to sync config: "+err); 
	}); 
	$rpc.wps.showpin().done(function(data){
		$scope.generatedPIN = data.pin; 
	}); 
	function wpsSuccess(){
		$scope.pairState = 'success'; 
		$scope.$apply(); 
		setTimeout(function(){
			$scope.showProgress = 0; 
			$scope.$apply(); 
		}, 2000); 
	}
	function wpsFail(){
		$scope.pairState = 'fail'; 
		$scope.$apply(); 
		setTimeout(function(){
			$scope.showProgress = 0; 
			$scope.$apply(); 
		}, 2000); 
	}
	$scope.save = function(){
		$uci.save(); 
	}
	$scope.onPairPBC = function(){
		$scope.showProgress = 1; 
		$scope.pairState = 'progress';
		$rpc.wps.pbc().done(function(){
			wpsSuccess(); 
		}).fail(function(){
			wpsFail(); 
		}); 
	}
	$scope.onPairUserPIN = function(){
		$scope.showProgress = 1; 
		$scope.pairState = 'progress';
		$rpc.wps.stapin({ pin: $scope.data.userPIN }).done(function(data){
			wpsSuccess(); 
		}).fail(function(){
			wpsFail(); 
		}).always(function(){
			$scope.data.userPIN = ""; 
			$scope.$apply(); 
		});  
	}
	$scope.onGeneratePIN = function(){
		$rpc.wps.genpin().done(function(data){
			$rpc.wps.setpin({pin: data.pin}).done(function(){
				$scope.generatedPIN = data.pin; 
				$scope.$apply(); 
			}); 
		}); 
	}
}); 
