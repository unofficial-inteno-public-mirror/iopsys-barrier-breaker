/*
 * juci - javascript universal client interface
 *
 * Project Author: Martin K. Schröder <mkschreder.uk@gmail.com>
 * 
 * Copyright (C) 2012-2013 Inteno Broadband Technology AB. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */
 
JUCI.app
.directive("luciLogin", function(){
		var plugin_root = $juci.module("core").plugin_root; 
		return {
			// accepted parameters for this tag
			scope: {
			}, 
			templateUrl: plugin_root+"/widgets/luci.login.html", 
			replace: true, 
			controller: "LoginControl",
			controllerAs: "ctrl"
		}; 
	})
	.controller("LoginControl", function($scope, $session, $state, 
		$window, $rpc, $location, $localStorage, gettext){
		$scope.form = { 
			"username": "", 
			"password": "", 
			"remember": 0, 
			"host": "" 
		}; 
		$scope.loggedIn = $session.isLoggedIn(); 
		$scope.errors = []; 
		$scope.showHost = 0; 
		if($rpc.local){
			$rpc.local.features().done(function(features){
				if(features.list) features.list.map(function(x){
					if(x.indexOf("rpcforward") == 0) {
						$scope.showHost = 1; 
						$scope.form.host = $localStorage.getItem("rpc_host")||""; 
					}
				}); 
				$scope.$apply(); 
			}); 
		}
		$scope.doLogin = function(){
			$scope.errors = []; 
			async.series([
				function(next){
					if($scope.form.host.length > 0){
						$rpc.local.set_rpc_host({"rpc_host": $scope.form.host})
						.done(function(){
							$localStorage.setItem("rpc_host", $scope.form.host); 
						})
						.always(function(){next();}); 
					} else {
						next(); 
					}
				}, 
				function(next){
					$session.login({
						"username": $scope.form.username, 
						"password": $scope.form.password, 
						"remember": $scope.form.remember
					}).done(function success(res){
						//$state.go("home", {}, {reload: true});
						$window.location.href="/"; 
					}).fail(function fail(res){
						$scope.errors.push(res); 
						$scope.errors.push(gettext("Please enter correct username and password!"));
						$scope.$apply(); 
					}); 
				}
			]); 
		}
		$scope.doLogout = function(){
			$session.logout().done(function(){
				console.log("Logged out!"); 
				//$state.go("home", {}, {reload: true});
				$window.location.href="/"; 
			}).fail(function(){
				alert("Error logging out!");
			});  
		}

	}); 
		