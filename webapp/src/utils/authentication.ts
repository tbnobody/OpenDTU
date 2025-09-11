import type { Emitter, EventType } from 'mitt';
import type { Router } from 'vue-router';

function getUserAuthData(): string | null {
    try {
        const user = JSON.parse(localStorage.getItem('user') || '');
        return user?.authdata || null;
    } catch {
        return null;
    }
}

export function authHeader(): Headers {
    // return authorization header with basic auth credentials
    const authdata = getUserAuthData();
    const headers = new Headers({ 'X-Requested-With': 'XMLHttpRequest' });
    if (authdata) {
        headers.append('Authorization', 'Basic ' + authdata);
    }
    return headers;
}

export function authUrl(): string {
    const authdata = getUserAuthData();
    if (authdata) {
        return encodeURIComponent(atob(authdata)).replace('%3A', ':') + '@';
    }
    return '';
}

export function logout() {
    // remove user from local storage to log user out
    localStorage.removeItem('user');
}

export function isLoggedIn(): boolean {
    return getUserAuthData() != null;
}

export function login(username: string, password: string) {
    const requestOptions = {
        method: 'GET',
        headers: {
            'X-Requested-With': 'XMLHttpRequest',
            Authorization: 'Basic ' + btoa(unescape(encodeURIComponent(username + ':' + password))),
        },
    };

    return fetch('/api/security/authenticate', requestOptions)
        .then(handleAuthResponse)
        .then((retVal) => {
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

export function handleResponse(
    response: Response,
    emitter: Emitter<Record<EventType, unknown>>,
    router: Router,
    ignore_error: boolean = false
) {
    return response.text().then((text) => {
        const data = text && JSON.parse(text);
        if (!response.ok) {
            if (response.status === 401) {
                // auto logout if 401 response returned from api
                logout();
                emitter.emit('logged-out');
                router.push({
                    path: '/login',
                    query: { returnUrl: router.currentRoute.value.fullPath },
                });
                return Promise.reject();
            }

            const error = {
                message: (data && data.message) || response.statusText,
                status: response.status || 0,
            };
            if (!ignore_error) {
                router.push({ name: 'Error', params: error });
            }
            return Promise.reject(error);
        }

        return data;
    });
}

function handleAuthResponse(response: Response) {
    return response.text().then((text) => {
        const data = text && JSON.parse(text);
        if (!response.ok) {
            if (response.status === 401) {
                // auto logout if 401 response returned from api
                logout();
            }

            const error = 'Invalid credentials';
            return Promise.reject(error);
        }

        return data;
    });
}
