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

$juci.module("vodaphone")
.directive("luciNavbar", function($http, $compile, $templateCache){
	var plugin_root = $juci.module("vodaphone").plugin_root; 
	return {
    priority: 100, // give it higher priority than built-in ng-click
		//templateUrl: plugin_root+"/widgets/vodaphone.navbar.html", 
		replace: true, 
		/*compile: function(tElement, tAtrrs) {
			templateLoader = $http.get(plugin_root + "/widgets/vodaphone.navbar.html", {cache: $templateCache})
			.success(function(html) {
				$templateCache.put("plugins/core/widgets/luci.navbar.html", html);
				tElement.html(html);
			});

      return function (scope, element, attrs) {
        templateLoader.then(function (templateText) {
					//alert(tElement.html()); 
          element.html($compile(tElement.html())(scope));
        });
      };
    }, */
    link: function(scope, element, attrs){
			var promise = $http.get(plugin_root + "/widgets/vodaphone.navbar.html", {cache: $templateCache}).success(function(html) {
				$templateCache.put("plugins/core/widgets/luci.navbar.html", html);
				element.html(html);
			}).then(function (response) {
				element.replaceWith($compile(element.html())(scope));
			});
		}
	}; 
}); 
