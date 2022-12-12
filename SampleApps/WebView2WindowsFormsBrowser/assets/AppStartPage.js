(async function () {
  async function uriToObject(uri) {
    const responseFromFetch = await fetch(uri);
    const responseAsText = await responseFromFetch.text();
    const response = JSON.parse(responseAsText);
    return response;
  }

  function parseQuery(query) {
    if (query.startsWith("?")) {
      query = query.substring(1);
    }

    return query.
        split("&").
        map(encodedNameValueStr => encodedNameValueStr.split("=")).
        reduce((resultObject, encodedNameValueArr) => {
          const nameValueArr = encodedNameValueArr.map(decodeURIComponent);
          resultObject[nameValueArr[0]] = nameValueArr[1];
          return resultObject;
        }, {});
  }

  const sdkReleasesNode = document.getElementById("sdkReleases");
  if (sdkReleasesNode) {
    const nugetInfoUri = "https://azuresearch-usnc.nuget.org/query?q=PackageID%3aMicrosoft.Web.WebView2&prerelease=true&semVerLevel=2.0.0";
    const nugetInfo = await uriToObject(nugetInfoUri);

    let versions = nugetInfo.data[0].versions;
    versions.reverse();
    versions.forEach(version => {
      const versionText = version.version;
      const aNode = document.createElement("a");
      aNode.href = "https://www.nuget.org/packages/Microsoft.Web.WebView2/" + versionText;
      aNode.textContent = "WebView2 SDK " + versionText;

      const itemNode = document.createElement("li");
      itemNode.appendChild(aNode);

      sdkReleasesNode.appendChild(itemNode);
    });
  }

  const query = parseQuery(location.search);
  const fillIds = ["sdkBuild", "runtimeVersion", "appPath", "runtimePath"];
  fillIds.forEach(id => {
    let content = query[id];
    if (content) {
      const maxContentLength = 100;
      if (content.length > maxContentLength) {
        content = "..." + content.substring(content.length - maxContentLength);
      }
      document.getElementById(id).textContent = content;
    }
  })
})();