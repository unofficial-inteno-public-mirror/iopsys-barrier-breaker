JUCI.app.controller("KthProject", function($scope, $rpc){
	$scope.items = ["item1", "item2"]; 
	
	$rpc.alljoyn.list().done(function(results){
		$scope.items = ["test"]; 
		$scope.$apply(); 
	}); 
}); 
