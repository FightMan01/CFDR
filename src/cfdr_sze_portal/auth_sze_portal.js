const g_keycloak = new Keycloak({
    url:      "https://idm.testportal.mathso.sze.hu",
    realm:    "hidalgo",
    clientId: "dashboard-ui"
});

try {
    const authenticated = await g_keycloak.init(
      {
        onLoad:                     'login-required',
        silentCheckSsoRedirectUri:  `${location.origin}/silent-check-sso.html`,
        checkLoginIframe:           0
      }
    );

    if (authenticated) {
        console.log('keycloak authentification successfull');
    } else {
        console.error('keycloak authentification failed');
    }
} catch (error) {
    console.error('keycloak adapter initialization failed:', error);
}

function auth_set_xhr_header(xhr) {
  g_keycloak.updateToken(30).then(function(refreshed) {
    if (refreshed) {
      xhr.setRequestHeader("Authorization", "Bearer " + g_keycloak.token);
    } else {
      console.error('keycloak token refresh failed');
    }
  });
  xhr.setRequestHeader("Authorization", "Bearer " + g_keycloak.token);
}
