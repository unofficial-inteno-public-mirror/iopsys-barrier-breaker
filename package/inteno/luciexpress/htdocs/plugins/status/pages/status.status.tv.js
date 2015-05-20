$juci.module("status")
.controller("StatusTVPageCtrl", function($scope, $rpc, gettext){
	$rpc.router.igmptable().done(function(result){
		if(!result.table) {
			$scope.$emit("error", gettext("Unable to retreive igmptable from device!")); 
			return; 
		} 
		$scope.igmptable = result.igmptable; 
	}); 
}); 
