const g_keycloak = new Keycloak({
    url:      "https://idm.testportal.mathso.sze.hu",
    realm:    "hidalgo",
    clientId: "dashboard-ui"
});

const g_keycloak_ready = init_sze_portal_auth();

async function init_sze_portal_auth() {
  try {
    const authenticated = await g_keycloak.init({
        onLoad:           'login-required',
        pkceMethod:       'S256',
        checkLoginIframe: false,
        scope:            'openid',
    });

    if (authenticated) {
      console.log('keycloak authentification successfull');
    } else {
      console.error('keycloak authentification failed');
    }
  } catch (error) {
    console.error('keycloak adapter initialization failed:', error);
    throw error;
  }
}

async function auth_set_xhr_header(xhr, request_url) {
  const resolved = new URL(request_url, location.href);
  if (resolved.origin !== location.origin || !resolved.pathname.startsWith("/project/")) {
    return;
  }

  await g_keycloak_ready;
  await g_keycloak.updateToken(30);

  if (!g_keycloak.token) {
    throw new Error('missing keycloak token');
  }

  xhr.setRequestHeader("Authorization", "Bearer " + g_keycloak.token);
}

function auth_resolve_request_url(url) {
  const resolved = new URL(url, location.href);

  if (resolved.origin !== location.origin) {
    return resolved.href;
  }

  const project_path = resolved.pathname.replace(/^\/+/, "");
  return `/project/${project_path}${resolved.search}${resolved.hash}`;
}
