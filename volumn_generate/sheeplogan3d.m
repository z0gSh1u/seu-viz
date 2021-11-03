clear all; clc

N = 64

shep = [
0 0 0 0.69 0.92 0.81 0 0 0 1
0 -0.0184 0 0.6624 0.8740 0.78 0 0 0 -0.8
0.22 0 0 0.11 0.31 0.22 -18 0 10 -0.2
-0.22 0 0 0.16 0.41 0.28 18 0 10 -0.2
0 0.35 -0.15 0.21 0.25 0.41 0 0 0 0.1
0 0.1 0.25 0.046 0.046 0.05 0 0 0 0.1
0 -0.1 0.25 0.046 0.046 0.05 0 0 0 0.1
-0.08 -0.6050 0 0.046 0.023 0.05 0 0 0 0.1
0 -0.606 0 0.023 0.023 0.02 0 0 0 0.1
0.06 -0.605 0 0.023 0.046 0.02 0 0 0 0.1
];

I = genHeadModel(shep, N);

filename = strcat('shep3d_',num2str(N),'.mat');
save(filename, 'I');

subplot(131),imshow(squeeze(I(N/2,:,:)), []);
subplot(132),imshow(squeeze(I(:,N/2,:)), []);
subplot(133),imshow(squeeze(I(:,:,N/2)), []);

function I = genHeadModel(shep, N)
    I = zeros(N, N, N);
    xe = shep(:, 1)';
    ye = shep(:, 2)';
    ze = shep(:, 3)';
    ae = shep(:, 4)';
    be = shep(:, 5)';
    ce = shep(:, 6)';
    phi = shep(:, 7)';
    gamma = shep(:, 8)';
    theta = shep(:, 9)';
    rho = shep(:, 10)';
    
    for k1 = 1:N
        for k2 = 1:N
            for k3 = 1:N
                phi_rad = phi * pi/180;
                gamma_rad = gamma * pi/180;
                theta_rad = theta * pi/180;
                x0 = (k1-N/2)/(N/2);
                y0 = (k2-N/2)/(N/2);
                z0 = (k3-N/2)/(N/2);
                cos_phi = cos(phi_rad); sin_phi = sin(phi_rad);
                cos_gamma = cos(gamma_rad); sin_gamma = sin(gamma_rad);
                cos_theta = cos(theta_rad); sin_theta = sin(theta_rad);
                
                T11 = cos_theta.*cos_phi-cos_gamma.*sin_phi.*sin_theta;
                T12 = cos_theta.*sin_phi+cos_gamma.*cos_phi.*sin_theta;
                T13 = sin_theta.*sin_gamma;
                T21 = -sin_theta.*cos_phi-cos_gamma.*sin_phi.*cos_theta;
                T22 = -sin_theta.*sin_phi+cos_gamma.*cos_phi.*cos_theta;
                T23 = cos_theta.*sin_gamma;
                T31 = sin_gamma.*sin_phi;
                T32 = -sin_gamma.*cos_phi;
                T33 = cos_gamma;
                
                XX = T11*x0+T12*y0+T13*z0;
                YY = T21*x0+T22*y0+T23*z0;
                ZZ = T31*x0+T32*y0+T33*z0;
                
                x = XX - xe;
                y = YY - ye;
                z = ZZ - ze;
                
                ellipsoid = x.^2./ae.^2+y.^2./be.^2+z.^2./ce.^2;
                ind = ellipsoid<=1;
                grayval = sum(rho.*ind);
                I(k1,k2,k3) = I(k1,k2,k3) + grayval;
            end
        end
    end
end