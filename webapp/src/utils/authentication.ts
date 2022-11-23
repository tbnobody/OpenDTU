import type { Emitter, EventType } from "mitt";
import type { Router } from "vue-router";

export function authHeader(): Headers {
    // return authorization header with basic auth credentials
    let user = null;
    try {
        user = JSON.parse(localStorage.getItem('user') || "");
    } catch { }

    const headers = new Headers();
    headers.append('X-Requested-With', 'XMLHttpRequest');
    if (user && user.authdata) {
        headers.append('Authorization', 'Basic ' + user.authdata);
    }
    return new Headers(headers);
}

export function authUrl(): string {
    let user = null;
    try {
        user = JSON.parse(localStorage.getItem('user') || "");
    } catch { }

    if (user && user.authdata) {
        return encodeURIComponent(atob(user.authdata)).replace("%3A", ":") + '@';
    }
    return "";
}

export function logout() {
    // remove user from local storage to log user out
    localStorage.removeItem('user');
}

export function isLoggedIn(): boolean {
    return (localStorage.getItem('user') != null);
}

export function login(username: String, password: String) {
    const requestOptions = {
        method: 'GET',
        headers: {
            'X-Requested-With': 'XMLHttpRequest',
            'Authorization': 'Basic ' + btoa(unescape(encodeURIComponent(username + ':' + password))),
        },
    };

    return fetch('/api/security/authenticate', requestOptions)
        .then(handleAuthResponse)
        .then(retVal => {
            // login successful if there's a user in the response
            if (retVal) {
                // store user details and basic auth credentials in local storage
                // to keep user logged in between page refreshes
                retVal.authdata = btoa(unescape(encodeURIComponent(username + ':' + password)));
                localStorage.setItem('user', JSON.stringify(retVal));
            }

            return retVal;
        });
}

export function handleResponse(response: Response, emitter: Emitter<Record<EventType, unknown>>, router: Router) {
    return response.text().then(text => {
        const data = text && JSON.parse(text);
        if (!response.ok) {
            if (response.status === 401) {
                // auto logout if 401 response returned from api
                logout();
                emitter.emit("logged-out");
                router.push({path: "/login", query: { returnUrl: router.currentRoute.value.fullPath }});
            }

            const error = (data && data.message) || response.statusText;
            return Promise.reject(error);
        }

        return data;
    });
}

function handleAuthResponse(response: Response) {
    return response.text().then(text => {
        const data = text && JSON.parse(text);
        if (!response.ok) {
            if (response.status === 401) {
                // auto logout if 401 response returned from api
                logout();
            }

            const error = "Invalid credentials";
            return Promise.reject(error);
        }

        return data;
    });
}