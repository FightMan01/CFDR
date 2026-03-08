const g_keycloak = new Keycloak({
    url:      "idm.testportal.mathso.sze.hu",
    realm:    "hidalgo",
    clientId: "dashboard-ui"
});

try {
    const authenticated = await g_keycloak.init(
      onLoad:                     'check-sso',
      silentCheckSsoRedirectUri:  `${location.origin}/silent-check-sso.html`
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
  await g_keycloak.updateToken(30);
  xhr.setRequestHeader("Authorization", "Bearer " + g_keycloak.token);
}
