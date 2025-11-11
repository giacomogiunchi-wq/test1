declare module 'express' {
  interface Request {
    [key: string]: any;
  }
  interface Response {
    status: (code: number) => Response;
    json: (body: any) => Response;
  }
  type NextFunction = (error?: any) => void;

  interface Router {
    use: (...handlers: any[]) => Router;
    get: (...handlers: any[]) => Router;
    post: (...handlers: any[]) => Router;
  }

  interface Express extends Router {
    listen: (...args: any[]) => any;
  }

  function express(): Express;
  namespace express {
    function Router(): Router;
    function json(options?: any): any;
  }

  export default express;
  export type { Request, Response, NextFunction, Router, Express };
}
